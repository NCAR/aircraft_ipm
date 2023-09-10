#!/bin/bash -ex


pkg=ipm
files="naiipm.cc naiipm.h ctrl.cc SConstruct"  # Files to include in package

key="<eol-prog@eol.ucar.edu>"

usage() {
    echo "Usage: ${1##*/} [-c] [-s] [-i repository ] [arch] [-h]"
    echo "-c: build in a chroot"
    echo "-s: sign the package files with key=$key"
    echo "-i: install them with reprepro to the repository"
    echo "default arch is i386"
    echo "-h: print usage statement"
    exit 1
}

# Set defaults
sign=false
arch=i386
use_chroot=false

# Parse command line arguments
while [ $# -gt 0 ]; do
    case $1 in
    -h)
        usage $0
        ;;
    -c)
        use_chroot=true
        ;;
    -s)
        sign=true
        ;;
    -i)
        shift
        repo=$1
        ;;
    i386)
        export CC=gcc
        arch=$1
        ;;
    *)
        usage $0
        ;;
    esac
    shift
done

if $use_chroot; then
    dist=$(lsb_release -c | awk '{print $2}')
    if [ $arch == i386 ]; then
        chr_name=${dist}-i386-sbuild
    else
        chr_name=${dist}-i386-cross-${arch}-sbuild
    fi
    if ! schroot -l | grep -F chroot:${chr_name}; then
        echo "chroot named ${chr_name} not found"
        exit 1
    fi
fi

sdir=$(dirname $0)
echo $sdir
cd $sdir
sdir=$PWD

# Sign the package files with key
args="-a$arch -sa"
karg=
if $sign; then
    export GPG_AGENT_INFO
    if [ -e $HOME/.gpg-agent-info ]; then
        . $HOME/.gpg-agent-info
    else
        echo "Warning: $HOME/.gpg-agent-info not found"
    fi
    karg=-k"$key"
else
    args="$args -us -uc"
fi

rm -f ${pkg}_*_$arch.changes

tar -xvf ${pkg}_0.1.orig.tar.xz  # Create basic configuration dir
cp ${files}  ${pkg}-0.1/.        # Add files

cd ${pkg}-0.1

# Make sure the tag has been created in the repo
if ! gitdesc=$(git describe --match "v0.1"); then
    echo "git describe failed, looking for a tag of the form v0.1"
    exit 1
fi

release=${gitdesc%-*}
release=${release#*-}

# Update the changelog
user=$(git log --max-count=1 --format="%an") || user=""
[ -z "${user}" ] && user="Unknown"
email=$(git log --max-count=1 --format="%ae") || email=""
[ -z "${email}" ] && email="unknown@ucar.edu"

rm -f debian/changelog
cat > debian/changelog << EOD
${pkg} (0.1-$release) stable; urgency=low

  * $(git log --max-count=1 --format="%s")

 -- ${user} <${email}>  $(git log --max-count=1 --format="%aD")
EOD
cat debian/initial_changelog >> debian/changelog

# Build the package
if $use_chroot; then
    echo "Starting schroot, which takes some time ..."
    schroot -c $chr_name --directory=$PWD << EOD
        set -e
        [ -f $HOME/.gpg-agent-info ] && . $HOME/.gpg-agent-info
        export GPG_AGENT_INFO
        debuild $args "$karg"
EOD
else
    debuild $args "$karg"
fi

cd ..

# Results
if [ -n "$repo" ]; then
    umask 0002

    echo "Build results:"
    ls
    echo ""

    changes=${pkg}_*_${arch}.changes
    echo "Changes file: $changes"
    cat $changes
    echo ""

    if [ $arch = armel ]; then
        flock $repo sh -e -c "
            reprepro -V -b $repo -C main --keepunreferencedfiles include jessie $changes"
    else
        # If arch is not armel, just install .debs
        debs=$(awk '/Files:/,/*/{print $5}' $changes | grep '.*\.deb$')
        flock $repo sh -e -c "
            reprepro -V -b $repo -C main -A $arch --keepunreferencedfiles includedeb jessie $debs"
    fi
    rm -f ${pkg}_*.build ${pkg}_*.dsc ${pkg}_*.debian.tar.xz ${pkg}_*.deb ${pkg}_*.changes
else
    echo "Results in $sdir"
fi

