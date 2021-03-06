#!/bin/sh -x
USAGE="USAGE: $0 [-H N] sana.exe iterations time-per-iter parallel-spec outdir {list of input networks}
parallel-spec is either a machine file for distrib_stdin, or '-parallel K' for K processes locally"

die() { echo "$USAGE" >&2; echo "FATAL ERROR: $*" >&2; exit 1
}
warn() { echo "Warning: $*" >&2
}
parse() { awk "BEGIN{print ($*)}" </dev/null
}
newlines() { /bin/awk '{for(i=1; i<=NF; i++) print $i}' "$@"
}
parallel() { /home/wayne/bin/bin.x86_64/parallel "$@"
}
lss() {
    LS=/bin/ls; SORT="sort -k +5nr"
    #LS=/usr/ucb/ls; SORT="sort +3nr"

    # List filenames in order of non-increasing byte size.
    # Don't descend directories unless the files in them are explicitly listed.

    # This rigamarole needs to be here.  Delete at your own risk.
    # The loop is fast; no external commands are called.

    # We can't just do "ls -ld $*" because if there are no arguments, all we get is
    #        drwx------   4 wayne        1024 Sep 24 15:46 .
    # which isn't what we intended.  Thus, if there are no arguments, we list
    # everything in the current directory; otherwise, list everything that's
    # listed on the command line, but don't descend any directories.  But now
    # we need to recognize if no *files* were listed, but options to ls(1) were
    # listed.  So we have to erase all the options before asking "was there
    # anything on the command line?"  The loop is fast; no external commands
    # are called.  Finally, we need to shift away the options and use "$@"
    # to pass the list of files, in case the filenames have spaces in them.

    #set -- `getopt aAbcFLnpqu "$@"`

    accept_opts=aAbcFLnpqul
    files=N
    opts=
    while :; do
	if [ "$#" = 0 ]; then 
	    break
	fi
	case "$1" in
	    -*) if getopt $accept_opts "$1" >/dev/null; then
		    opts="$opts $1"
		    shift
		else
		    # getopt prints the error message for us
		    exit 1
		fi
		;;
	    --) break;;
	    *)  files=Y
		break   # files begin here
		;;
	esac
    done

    case "$files" in
	N) $LS -l $opts | $SORT ;;
	Y) $LS -ld $opts "$@" | $SORT ;;
    esac
}

export PARALLEL='distrib_stdin.new -s 0.03 -f $MACHINES'

TMPDIR=/tmp/overseer.$$
trap "/bin/rm -rf $TMPDIR; exit" 0 1 2 3 15
mkdir $TMPDIR

parallel_delay() {
    cat > $TMPDIR/pd
    head -1 $TMPDIR/pd | tee /dev/tty | sh &
    sleep 1
    until [ -f "$1" ]; do sleep 1; done
    tail -n +2 $TMPDIR/pd | tee /dev/tty | eval $PARALLEL
}

HILLCLIMB=0
TYPES=false
TYPEargs=''
while echo "X$1" | grep '^X-' >/dev/null; do
    case "$1" in
    -H) HILLCLIMB=$2; shift;; # in addition to the shift that'll happen below
    -nodes-have-types) TYPES=true; TYPEargs='-nodes-have-types -lock-same-names -lock /dev/null';;
    -*) die "unknown option '$1'";;
    esac
    shift
done
SANA="$1"
SANAs3=./sana.s3
ITER_EXPR="$2"
T_ITER="$3"
MACHINES="$4"
OUTDIR="$5"
NAME=`basename "$OUTDIR"`
export SANA ITER_EXPR NAME
shift 5

[ -x "$SANA" ] || die "first argument '$SANA' must be an executable file"
case "$MACHINES" in
    -parallel*) PARALLEL='parallel -s bash '`echo $MACHINES | awk '{print $NF-1}'`;;
    *) [ -f "$MACHINES" ] || die "4th argument '$MACHINES' must be '-parallel N' or a machine list for distrib_stdin";;
esac
ITER=`parse "$ITER_EXPR"` || die "'$ITER_EXPR': cannot figure out iteration count"
if [ -d "$OUTDIR" ]; then
    warn "outdir '$OUTDIR' already exists; continuing"
    MAX_NODES=`lss $OUTDIR/dir-init/*.align | awk 'NR==1{print $NF}' | xargs cat | wc -l` || die "Couldn't figure out value of MAX_NODES from '$OUTDIR/dir-init/*.align'"
fi

mkdir -p "$OUTDIR"/dir-init || die "Cannot make outdir '$OUTDIR'"

# NOTE: REMAINDER OF THE COMMAND LINE IS ALL THE INPUT NETWORKS

# Create initial random alignment, which also tells us the number of nodes.
[ -f "$OUTDIR"/dir-init/group.multiAlign ] || ./random-multi-alignment.sh $TYPES "$OUTDIR"/dir-init "$@"
MAX_NODES=`lss $OUTDIR/dir-init/*.align | awk 'NR==1{print $NF}' | xargs cat | wc -l`

mkdir -p $OUTDIR/dir0
#[ -f $OUTDIR/dir0/$NAME-shadow0.gw ] || (./createShadow -s$MAX_NODES "$@" $OUTDIR/dir-init/*-shadow.out >$OUTDIR/dir0/$NAME-shadow0.gw) || die "$NAME-shadow0.gw network creation failed"
[ -f $OUTDIR/dir0/$NAME-shadow0.el ] || (./createShadow -s29263 --nodes-have-types --shadowNames Jurisica/networks/SHADOW.all.txt "$@" $OUTDIR/dir-init/*-shadow.align > $OUTDIR/dir0/$NAME-shadow0.el) || die "$NAME-shadow0.el network creation failed"

# Add 10 dummy "holes" for mRNAs at the end of the shadow network, each with weight 0 edges.
yes | head -26 | (awk '{printf "DUMMY\tmRNA-%c\t0\n", 64+NR}' >> $OUTDIR/dir0/$NAME-shadow0.el)
mv $OUTDIR/dir-init/*-shadow.align $OUTDIR/dir-init/*-shadow.out $OUTDIR/dir0

echo -n "Computing SES denominator..."
SES_DENOM=`numEdges "$@" | sort -n |
	awk '{m[NR-1]=$1}END{for(i=0;i<NR;i++) if(NR-i>=1){D+=(NR-i)^2*m[i];for(j=i+1;j<NR;j++)m[j]-=m[i]}; print D}'`
echo Denominator for SES score is $SES_DENOM
export SES_DENOM
# Now get temperature schedule and SES denominator (the latter requires *.out files so don't use -scheduleOnly)
mkdir -p $OUTDIR/dir-init
/bin/rm -rf networks/$NAME-shadow0
touch $OUTDIR/dir-init/schedule.tsv $OUTDIR/dir-init/tdecay.txt
if true; then
    while [ `awk '{printf "%s.stdout\n", $1}' $OUTDIR/dir-init/schedule.tsv | tee $OUTDIR/dir-init/schedule.done | wc -l` -lt `echo "$@" | wc -w` ]; do
	/bin/rm -rf networks/$NAME-shadow0
	mkdir    -p networks/$NAME-shadow0; (cd networks/$NAME-shadow0; ln -s /var/preserve/autogenerated .)
	ls "$@" | awk '{file=$0;gsub(".*/",""); gsub(".el$",""); gsub(".gw$",""); printf "mkdir -p '$OUTDIR/dir-init';'"$SANA $TYPEargs"' -multi-iteration-only -t 0 -s3 0 -ses 1 -fg1 %s -fg2 '$OUTDIR/dir0/$NAME-shadow0.el' -o '$OUTDIR'/dir-init/%s >'$OUTDIR'/dir-init/%s.stdout\n", file,$0,$0}' | fgrep -v -f $OUTDIR/dir-init/schedule.done | tee $OUTDIR/dir-init/jobs.txt | parallel_delay networks/$NAME-shadow0/autogenerated/$NAME-shadow0_NodeNameToIndexMap.bin
	awk '/^Initial temperature/{Tinit[FILENAME]=$4}/^Final temperature /{Tfinal[FILENAME]=$4}/^tdecay:/{Tdecay[FILENAME]=$2}END{for(i in Tinit)print i,Tinit[i],Tfinal[i],Tdecay[i]}' $OUTDIR/dir-init/*.stdout | sed -e "s,$OUTDIR/dir-init/,," -e 's/\.stdout//' > $OUTDIR/dir-init/tinitial-final.txt
	echo 'name	tinitial	tfinal	tdecay' | tee $OUTDIR/dir-init/schedule.tsv
	sed 's/ /	/g' $OUTDIR/dir-init/tinitial-final.txt | tee -a $OUTDIR/dir-init/schedule.tsv
    done
else
    echo 'name	tinitial	tfinal	tdecay' | tee $OUTDIR/dir-init/schedule.tsv
    ls "$@" | awk '{file=$0;gsub(".*/",""); gsub(".el$",""); gsub(".gw$","");printf "%s	40	1e-10	5\n",$1}' | tee -a $OUTDIR/dir-init/schedule.tsv
fi

for i in `integers $ITER` `integers $ITER $HILLCLIMB`
do
    #echo UGLY HACK AROUND SERIALIZATION BUG
    #/bin/rm -rf /var/preserve/autogenerated/*
    #echo UGLY HACK AROUND SERIALIZATION BUG

    echo -n ---- ITER $i -----
    if [ $i -ge $ITER ]; then echo -n "(HillClimb) "; T_ITER=1; fi
    i1=`expr $i + 1`
    if [ -f  $OUTDIR/dir$i1/$NAME-shadow$i1.el ]; then continue; fi
    if [ -f  $OUTDIR/dir$i1/jobs-done.txt ]; then
       if [ `wc -l < $OUTDIR/dir$i1/jobs-done.txt` -eq `echo "$@" | wc -w` ]; then
	    continue
	fi
    fi
    /bin/rm -rf $OUTDIR/dir$i1
    mkdir -p $OUTDIR/dir$i1
    for g
    do
	bg=`basename $g .gw`
	bg=`basename $bg .el`
	echo $bg-shadow.align >> $OUTDIR/dir$i1/expected-outFiles.txt
	named-col-prog 'if(name!="'$bg'")next; i='$i';ITER='$ITER'; e0=log(tinitial);e1=log(tfinal);printf "mkdir -p '$OUTDIR/dir$i1';'"$SANA $TYPEargs"' -multi-iteration-only -s3 0 -ses 1 -t '$T_ITER' -fg1 '$g' -fg2 '$OUTDIR/dir$i/$NAME-shadow$i.el' -tinitial %g -tdecay %g -o '$OUTDIR/dir$i1/$bg-shadow' >'$OUTDIR/dir$i1/$bg-shadow.stdout' -startalignment '$OUTDIR/dir$i/$bg-shadow.align'\n", tinitial*exp((e1-e0)*i/ITER),tdecay/ITER' $OUTDIR/dir-init/schedule.tsv
    done | sort -u > $OUTDIR/dir$i/jobs.txt
    /bin/rm -rf networks/$NAME-shadow$i
    mkdir -p networks/$NAME-shadow$i; (cd networks/$NAME-shadow$i; ln -s /var/preserve/autogenerated .)
    while [ `ls $OUTDIR/dir$i1 | fgrep -f $OUTDIR/dir$i1/expected-outFiles.txt | tee $OUTDIR/dir$i/jobs-done.txt | wc -l` -lt `echo "$@" | wc -w` ]; do
	sed 's/\.out\.align/.stdout/' $OUTDIR/dir$i/jobs-done.txt | fgrep -v -f - $OUTDIR/dir$i/jobs.txt | timed-run -`parse "60*($T_ITER+20)"` parallel_delay networks/$NAME-shadow$i/autogenerated/$NAME-shadow${i}_NodeNameToIndexMap.bin || # wait until that directory is created before parallel'ing
	    die "timed-run failed, exiting"
    done
    ./shadow2align.sh $OUTDIR/dir$i1/*.align > $OUTDIR/dir$i1/multiAlign.tsv
    #./createShadow -s$MAX_NODES "$@" $OUTDIR/dir$i1/*-shadow.out >$OUTDIR/dir$i1/$NAME-shadow$i1.gw || die "$OUTDIR/dir$i1/$NAME-shadow$i1.gw network creation failed"
    ./createShadow -s29263 --nodes-have-types --shadowNames Jurisica/networks/SHADOW.all.txt "$@" $OUTDIR/dir$i1/*-shadow.align > $OUTDIR/dir$i1/$NAME-shadow$i1.el || die "$OUTDIR/dir$i1/$NAME-shadow$i1.el network creation failed"
    yes | head | (awk '{printf "DUMMY\tmRNA-%c\t0\n", 64+NR}' >> $OUTDIR/dir$i1/$NAME-shadow$i1.el)
    awk '{gsub("[|{}]","")}$3>1{sum2+=$3^2}END{printf " SES %g\n", sum2/'$SES_DENOM'}' $OUTDIR/dir$i1/$NAME-shadow$i1.el
done
echo "Computing CIQ... may take awhile..."
./CIQ.sh $OUTDIR/dir$i1/multiAlign.tsv `echo "$@" | newlines | sed 's/\.gw/.el/'`
