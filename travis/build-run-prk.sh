set -e
set -x

os=`uname`
TRAVIS_ROOT="$1"
PRK_TARGET="$2"
# Travis exports this
PRK_COMPILER="$CC"

MPI_IMPL=mpich

echo "PRKVERSION=\"'2.16'\"" > common/make.defs

case "$os" in
    Darwin)
        # Homebrew should put MPI here...
        export MPI_ROOT=/usr/local
        ;;
    Linux)
        export MPI_ROOT=$TRAVIS_ROOT/$MPI_IMPL
        ;;
esac

case "$PRK_TARGET" in
    allpython)
        echo "Python"
        which python
        python --version
        export PRK_TARGET_PATH=PYTHON
        export PRK_SUFFIX=.py
        python $PRK_TARGET_PATH/p2p$PRK_SUFFIX             10 1024 1024
        python $PRK_TARGET_PATH/stencil$PRK_SUFFIX         10 1000
        python $PRK_TARGET_PATH/transpose$PRK_SUFFIX       10 1024
        export PRK_SUFFIX=-numpy.py
        python $PRK_TARGET_PATH/p2p-numpy$PRK_SUFFIX       10 1024 1024
        python $PRK_TARGET_PATH/stencil-numpy$PRK_SUFFIX   10 1000
        python $PRK_TARGET_PATH/transpose-numpy$PRK_SUFFIX 10 1024
        ;;
    allserial)
        echo "Serial"
        echo "CC=$PRK_COMPILER -std=c99" >> common/make.defs
        make $PRK_TARGET
        export PRK_TARGET_PATH=C89
        export PRK_SUFFIX=-ser
        $PRK_TARGET_PATH/p2p$PRK_SUFFIX       10 1024 1024
        $PRK_TARGET_PATH/stencil$PRK_SUFFIX   10 1000
        $PRK_TARGET_PATH/transpose$PRK_SUFFIX 10 1024 32
        $PRK_TARGET_PATH/reduce$PRK_SUFFIX    10 1024
        $PRK_TARGET_PATH/random$PRK_SUFFIX    64 10 16384
        $PRK_TARGET_PATH/nstream$PRK_SUFFIX   10 16777216 32
        $PRK_TARGET_PATH/sparse$PRK_SUFFIX    10 10 5
        $PRK_TARGET_PATH/dgemm$PRK_SUFFIX     10 1024 32
        $PRK_TARGET_PATH/pic$PRK_SUFFIX       10 1000 1000000 1 2 GEOMETRIC 0.99
        $PRK_TARGET_PATH/pic$PRK_SUFFIX       10 1000 1000000 0 1 SINUSOIDAL
        $PRK_TARGET_PATH/pic$PRK_SUFFIX       10 1000 1000000 1 0 LINEAR 1.0 3.0
        $PRK_TARGET_PATH/pic$PRK_SUFFIX       10 1000 1000000 1 0 PATCH 0 200 100 200
        ;;
    allfortran)
        echo "Fortran"
        case "$CC" in
            gcc)
                for gccversion in "-5" "-5.3" "-5.2" "-5.1" "-4.9" "-4.8" "-4.7" "-4.6" "" ; do
                    if [ -f "`which gfortran$gccversion`" ]; then
                        export PRK_FC="gfortran$gccversion"
                        echo "Found GCC Fortran: $PRK_FC"
                        break
                    fi
                done
                if [ "x$PRK_FC" == "x" ] ; then
                    echo "No Fortran compiler found!"
                    exit 9
                fi
                export PRK_FC="$PRK_FC -std=f2008 -cpp"
                echo "FC=$PRK_FC\nOPENMPFLAG=-fopenmp\nCOARRAYFLAG=-fcoarray=single" >> common/make.defs
                ;;
            clang)
                echo "LLVM Fortran is not supported."
                exit 9
                echo "FC=flang" >> common/make.defs
                ;;
        esac
        make $PRK_TARGET
        export PRK_TARGET_PATH=FORTRAN
        export PRK_SUFFIX=
        $PRK_TARGET_PATH/p2p$PRK_SUFFIX       10 1024 1024
        $PRK_TARGET_PATH/stencil$PRK_SUFFIX   10 1000
        $PRK_TARGET_PATH/transpose$PRK_SUFFIX 10 1024 1
        $PRK_TARGET_PATH/transpose$PRK_SUFFIX 10 1024 32
        # pretty versions do not support tiling...
        export PRK_SUFFIX=-pretty
        #$PRK_TARGET_PATH/p2p$PRK_SUFFIX       10 1024 1024
        $PRK_TARGET_PATH/stencil$PRK_SUFFIX   10 1000
        $PRK_TARGET_PATH/transpose$PRK_SUFFIX 10 1024
        export OMP_NUM_THREADS=2
        export PRK_SUFFIX=-omp
        #$PRK_TARGET_PATH/p2p$PRK_SUFFIX       10 1024 1024 # not threaded yet
        $PRK_TARGET_PATH/stencil$PRK_SUFFIX   10 1000
        $PRK_TARGET_PATH/transpose$PRK_SUFFIX 10 1024 1
        $PRK_TARGET_PATH/transpose$PRK_SUFFIX 10 1024 32
        # FIXME: only testing with a single image right now.
        export PRK_SUFFIX=-coarray
        $PRK_TARGET_PATH/p2p$PRK_SUFFIX       10 1024 1024
        $PRK_TARGET_PATH/stencil$PRK_SUFFIX   10 1000
        $PRK_TARGET_PATH/transpose$PRK_SUFFIX 10 1024 1
        $PRK_TARGET_PATH/transpose$PRK_SUFFIX 10 1024 32
        ;;
    allopenmp)
        echo "OpenMP"
        echo "CC=$PRK_COMPILER -std=c99\nOPENMPFLAG=-fopenmp" >> common/make.defs
        make $PRK_TARGET
        export PRK_TARGET_PATH=C89
        export PRK_SUFFIX=-omp
        export OMP_NUM_THREADS=4
        $PRK_TARGET_PATH/p2p$PRK_SUFFIX       $OMP_NUM_THREADS 10 1024 1024
        $PRK_TARGET_PATH/stencil$PRK_SUFFIX   $OMP_NUM_THREADS 10 1000
        $PRK_TARGET_PATH/transpose$PRK_SUFFIX $OMP_NUM_THREADS 10 1024 32
        $PRK_TARGET_PATH/reduce$PRK_SUFFIX    $OMP_NUM_THREADS 10 1024
        $PRK_TARGET_PATH/random$PRK_SUFFIX    $OMP_NUM_THREADS 64 10 16384
        $PRK_TARGET_PATH/nstream$PRK_SUFFIX   $OMP_NUM_THREADS 10 16777216 32
        $PRK_TARGET_PATH/sparse$PRK_SUFFIX    $OMP_NUM_THREADS 10 10 5
        $PRK_TARGET_PATH/dgemm$PRK_SUFFIX     $OMP_NUM_THREADS 10 1024 32
        $PRK_TARGET_PATH/global$PRK_SUFFIX    $OMP_NUM_THREADS 10 16384
        $PRK_TARGET_PATH/private$PRK_SUFFIX   $OMP_NUM_THREADS 16777216
        $PRK_TARGET_PATH/shared$PRK_SUFFIX    $OMP_NUM_THREADS 16777216 1024
        # random is broken right now it seems
        $PRK_TARGET_PATH/random$PRK_SUFFIX    $OMP_NUM_THREADS 10 16384 32
        ;;
    allmpi1)
        echo "MPI-1"
        echo "MPICC=$MPI_ROOT/bin/mpicc" >> common/make.defs
        make $PRK_TARGET
        export PRK_TARGET_PATH=C89
        export PRK_SUFFIX=-mpi1
        export PRK_MPI_PROCS=4
        export PRK_LAUNCHER=$MPI_ROOT/bin/mpirun
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/p2p$PRK_SUFFIX       10 1024 1024
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/stencil$PRK_SUFFIX   10 1000
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/transpose$PRK_SUFFIX 10 1024 32
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/reduce$PRK_SUFFIX    10 1024
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/random$PRK_SUFFIX    64 10 16384
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/nstream$PRK_SUFFIX   10 16777216 32
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/sparse$PRK_SUFFIX    10 10 5
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/dgemm$PRK_SUFFIX     10 1024 32
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/global$PRK_SUFFIX    10 16384
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/pic$PRK_SUFFIX       10 1000 1000000 1 2 GEOMETRIC 0.99
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/pic$PRK_SUFFIX       10 1000 1000000 0 1 SINUSOIDAL
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/pic$PRK_SUFFIX       10 1000 1000000 1 0 LINEAR 1.0 3.0
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/pic$PRK_SUFFIX       10 1000 1000000 1 0 PATCH 0 200 100 200
        ;;
    allmpio*mp)
        echo "MPI+OpenMP"
        echo "MPICC=$MPI_ROOT/bin/mpicc\nOPENMPFLAG=-fopenmp" >> common/make.defs
        make $PRK_TARGET
        export PRK_TARGET_PATH=C89
        export PRK_MPI_PROCS=2
        export OMP_NUM_THREADS=2
        export PRK_LAUNCHER=$MPI_ROOT/bin/mpirun
        export PRK_SUFFIX=-mpiomp
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/p2p$PRK_SUFFIX       $OMP_NUM_THREADS 10 1024 1024
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/stencil$PRK_SUFFIX   $OMP_NUM_THREADS 10 1000
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/transpose$PRK_SUFFIX $OMP_NUM_THREADS 10 1024 32
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/nstream$PRK_SUFFIX   $OMP_NUM_THREADS 10 16777216 32
        ;;
    allmpirma)
        echo "MPI-RMA"
        echo "MPICC=$MPI_ROOT/bin/mpicc" >> common/make.defs
        make $PRK_TARGET
        export PRK_TARGET_PATH=C89
        export PRK_SUFFIX=-mpirma
        export PRK_MPI_PROCS=4
        export PRK_LAUNCHER=$MPI_ROOT/bin/mpirun
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/p2p$PRK_SUFFIX       10 1024 1024
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/stencil$PRK_SUFFIX   10 1000
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/transpose$PRK_SUFFIX 10 1024 32
        ;;
    allmpishm)
        echo "MPI+MPI"
        echo "MPICC=$MPI_ROOT/bin/mpicc" >> common/make.defs
        make $PRK_TARGET
        export PRK_TARGET_PATH=C89
        export PRK_SUFFIX=-mpishm
        export PRK_MPI_PROCS=4
        export PRK_MPISHM_RANKS=$(($PRK_MPI_PROCS/2))
        export PRK_LAUNCHER=$MPI_ROOT/bin/mpirun
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/p2p$PRK_SUFFIX                         10 1024 1024
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/stencil$PRK_SUFFIX   $PRK_MPISHM_RANKS 10 1000
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/transpose$PRK_SUFFIX $PRK_MPISHM_RANKS 10 1024 32
        ;;
    allshmem)
        echo "SHMEM"
        # This should be fixed by rpath (https://github.com/regrant/sandia-shmem/issues/83)
        export LD_LIBRARY_PATH=$TRAVIS_ROOT/sandia-openshmem/lib:$TRAVIS_ROOT/libfabric/lib:$LD_LIBRARY_PATH
        export SHMEM_ROOT=$TRAVIS_ROOT/sandia-openshmem
        echo "OSHCC=$SHMEM_ROOT/bin/oshcc" >> common/make.defs
        make $PRK_TARGET
        export PRK_TARGET_PATH=C89
        export PRK_SUFFIX=-shmem
        export PRK_SHMEM_PROCS=4
        export OSHRUN_LAUNCHER=$TRAVIS_ROOT/hydra/bin/mpirun
        export PRK_LAUNCHER=$SHMEM_ROOT/bin/oshrun
        $PRK_LAUNCHER -n $PRK_SHMEM_PROCS $PRK_TARGET_PATH/p2p$PRK_SUFFIX       10 1024 1024
        $PRK_LAUNCHER -n $PRK_SHMEM_PROCS $PRK_TARGET_PATH/stencil$PRK_SUFFIX   10 1000
        $PRK_LAUNCHER -n $PRK_SHMEM_PROCS $PRK_TARGET_PATH/transpose$PRK_SUFFIX 10 1024 32
        ;;
    allupc)
        echo "UPC"
        export PRK_UPC_PROCS=4
        case "$UPC_IMPL" in
            gupc)
                case "$CC" in
                    gcc)
                        # If building from source (impossible)
                        #export UPC_ROOT=$TRAVIS_ROOT/gupc
                        # If installing deb file
                        export UPC_ROOT=$TRAVIS_ROOT/gupc/usr/local/gupc
                        ;;
                    clang)
                        echo "Clang UPC is not supported."
                        exit 9
                        export UPC_ROOT=$TRAVIS_ROOT/clupc
                        ;;
                esac
                echo "UPCC=$UPC_ROOT/bin/upc" >> common/make.defs
                export PRK_LAUNCHER=""
                export PRK_LAUNCHER_ARGS="-n $PRK_UPC_PROCS"
                make $PRK_TARGET
                ;;
            bupc)
                export UPC_ROOT=$TRAVIS_ROOT/bupc-$CC
                echo "UPCC=$UPC_ROOT/bin/upcc" >> common/make.defs
                # -N $nodes -n UPC threads -c $cores_per_node
                # -localhost is only for UDP
                case "$GASNET_CONDUIT" in
                    udp)
                        export PRK_LAUNCHER="$UPC_ROOT/bin/upcrun -N 1 -n $PRK_UPC_PROCS -c $PRK_UPC_PROCS -localhost"
                        ;;
                    ofi)
                        export GASNET_SSH_SERVERS="localhost"
                        export LD_LIBRARY_PATH="$TRAVIS_ROOT/libfabric/lib:$LD_LIBRARY_PATH"
                        export PRK_LAUNCHER="$UPC_ROOT/bin/upcrun -v -N 1 -n $PRK_UPC_PROCS -c $PRK_UPC_PROCS"
                        ;;
                    mpi)
                        # see if this is causing Mac tests to hang
                        export MPICH_ASYNC_PROGRESS=1
                        # so that upcrun can find mpirun - why it doesn't cache this from build is beyond me
                        export PATH="$TRAVIS_ROOT/$MPI_IMPL/bin:$PATH"
                        export PRK_LAUNCHER="$UPC_ROOT/bin/upcrun -N 1 -n $PRK_UPC_PROCS -c $PRK_UPC_PROCS"
                        ;;
                    *)
                        export PRK_LAUNCHER="$UPC_ROOT/bin/upcrun -N 1 -n $PRK_UPC_PROCS -c $PRK_UPC_PROCS"
                        ;;
                esac
                make $PRK_TARGET default_opt_flags="-Wc,-O3"
                ;;
        esac
        export PRK_TARGET_PATH=C99
        export PRK_SUFFIX=-upc
        $PRK_LAUNCHER $PRK_TARGET_PATH/p2p$PRK_SUFFIX       $PRK_LAUNCHER_ARGS 10 1024 1024
        $PRK_LAUNCHER $PRK_TARGET_PATH/stencil$PRK_SUFFIX   $PRK_LAUNCHER_ARGS 10 1000
        $PRK_LAUNCHER $PRK_TARGET_PATH/transpose$PRK_SUFFIX $PRK_LAUNCHER_ARGS 10 1024 32
        ;;
    allcharm++)
        echo "Charm++"
        os=`uname`
        case "$os" in
            Darwin)
                export CHARM_ROOT=$TRAVIS_ROOT/charm-6.7.0/netlrts-darwin-x86_64-smp
                ;;
            Linux)
                #export CHARM_ROOT=$TRAVIS_ROOT/charm-6.7.0/netlrts-linux-x86_64
                export CHARM_ROOT=$TRAVIS_ROOT/charm-6.7.0/netlrts-linux-x86_64-smp
                #export CHARM_ROOT=$TRAVIS_ROOT/charm-6.7.0/multicore-linux64
                ;;
        esac
        echo "CHARMTOP=$CHARM_ROOT" >> common/make.defs
        make $PRK_TARGET
        export PRK_TARGET_PATH=CHARM++
        export PRK_SUFFIX=-charm
        export PRK_CHARM_PROCS=4
        export PRK_LAUNCHER=$CHARM_ROOT/bin/charmrun
        export PRK_LAUNCHER_ARGS="+p$PRK_CHARM_PROCS ++local"
        # For Charm++, the last argument is the overdecomposition factor -->     \|/
        $PRK_LAUNCHER $PRK_TARGET_PATH/p2p$PRK_SUFFIX       $PRK_LAUNCHER_ARGS 10 1024 1024  1
        $PRK_LAUNCHER $PRK_TARGET_PATH/stencil$PRK_SUFFIX   $PRK_LAUNCHER_ARGS 10 1000       1
        $PRK_LAUNCHER $PRK_TARGET_PATH/transpose$PRK_SUFFIX $PRK_LAUNCHER_ARGS 10 1024 32    1
        ;;
    allampi)
        echo "Adaptive MPI (AMPI)"
        os=`uname`
        case "$os" in
            Darwin)
                export CHARM_ROOT=$TRAVIS_ROOT/charm-6.7.0/netlrts-darwin-x86_64-smp
                ;;
            Linux)
                #export CHARM_ROOT=$TRAVIS_ROOT/charm-6.7.0/netlrts-linux-x86_64
                export CHARM_ROOT=$TRAVIS_ROOT/charm-6.7.0/netlrts-linux-x86_64-smp
                #export CHARM_ROOT=$TRAVIS_ROOT/charm-6.7.0/multicore-linux64
                ;;
        esac
        echo "CHARMTOP=$CHARM_ROOT" >> common/make.defs
        make $PRK_TARGET
        export PRK_TARGET_PATH=C89
        export PRK_SUFFIX=-ampi
        export PRK_CHARM_PROCS=4
        export PRK_LAUNCHER=$CHARM_ROOT/bin/charmrun
        export PRK_LAUNCHER_ARGS="+p$PRK_CHARM_PROCS +vp$PRK_CHARM_PROCS +isomalloc_sync ++local"
        $PRK_LAUNCHER $PRK_TARGET_PATH/p2p$PRK_SUFFIX       $PRK_LAUNCHER_ARGS 10 1024 1024
        $PRK_LAUNCHER $PRK_TARGET_PATH/stencil$PRK_SUFFIX   $PRK_LAUNCHER_ARGS 10 1000
        $PRK_LAUNCHER $PRK_TARGET_PATH/transpose$PRK_SUFFIX $PRK_LAUNCHER_ARGS 10 1024 32
        # FIXME Fails with timeout - bug in AMPI?
        #$PRK_LAUNCHER $PRK_TARGET_PATH/reduce$PRK_SUFFIX    $PRK_LAUNCHER_ARGS 10 1024
        # FIXME This one hangs - bug in AMPI?
        #$PRK_LAUNCHER $PRK_TARGET_PATH/random$PRK_SUFFIX    $PRK_LAUNCHER_ARGS 64 10 16384
        $PRK_LAUNCHER $PRK_TARGET_PATH/nstream$PRK_SUFFIX   $PRK_LAUNCHER_ARGS 10 16777216 32
        $PRK_LAUNCHER $PRK_TARGET_PATH/sparse$PRK_SUFFIX    $PRK_LAUNCHER_ARGS 10 10 5
        $PRK_LAUNCHER $PRK_TARGET_PATH/dgemm$PRK_SUFFIX     $PRK_LAUNCHER_ARGS 10 1024 32 1
        $PRK_LAUNCHER $PRK_TARGET_PATH/global$PRK_SUFFIX    $PRK_LAUNCHER_ARGS 10 16384
        #$PRK_LAUNCHER $PRK_TARGET_PATH/pic$PRK_SUFFIX       $PRK_LAUNCHER_ARGS 10 1000 1000000 1 2 GEOMETRIC 0.99
        #$PRK_LAUNCHER $PRK_TARGET_PATH/pic$PRK_SUFFIX       $PRK_LAUNCHER_ARGS 10 1000 1000000 0 1 SINUSOIDAL
        #$PRK_LAUNCHER $PRK_TARGET_PATH/pic$PRK_SUFFIX       $PRK_LAUNCHER_ARGS 10 1000 1000000 1 0 LINEAR 1.0 3.0
        #$PRK_LAUNCHER $PRK_TARGET_PATH/pic$PRK_SUFFIX       $PRK_LAUNCHER_ARGS 10 1000 1000000 1 0 PATCH 0 200 100 200
        ;;
    allfgmpi)
        echo "Fine-Grain MPI (FG-MPI)"
        export FGMPI_ROOT=$TRAVIS_ROOT/fgmpi
        echo "FGMPITOP=$FGMPI_ROOT\nFGMPICC=$FGMPI_ROOT/bin/mpicc" >> common/make.defs
        make $PRK_TARGET
        export PRK_TARGET_PATH=C89
        export PRK_SUFFIX=-fgmpi
        export PRK_MPI_PROCS=2
        export PRK_FGMPI_THREADS=2
        export PRK_LAUNCHER=$FGMPI_ROOT/bin/mpiexec
        $PRK_LAUNCHER -np $PRK_MPI_PROCS -nfg $PRK_FGMPI_THREADS $PRK_TARGET_PATH/p2p$PRK_SUFFIX       10 1024 1024
        # FIXME Fails with:
        # ERROR: rank 2 has work tile smaller then stencil radius
        $PRK_LAUNCHER -np $PRK_MPI_PROCS -nfg $PRK_FGMPI_THREADS $PRK_TARGET_PATH/stencil$PRK_SUFFIX   10 1000
        $PRK_LAUNCHER -np $PRK_MPI_PROCS -nfg $PRK_FGMPI_THREADS $PRK_TARGET_PATH/transpose$PRK_SUFFIX 10 1024 32
        $PRK_LAUNCHER -np $PRK_MPI_PROCS -nfg $PRK_FGMPI_THREADS $PRK_TARGET_PATH/reduce$PRK_SUFFIX    10 16777216
        $PRK_LAUNCHER -np $PRK_MPI_PROCS -nfg $PRK_FGMPI_THREADS $PRK_TARGET_PATH/nstream$PRK_SUFFIX   10 16777216 32
        $PRK_LAUNCHER -np $PRK_MPI_PROCS -nfg $PRK_FGMPI_THREADS $PRK_TARGET_PATH/sparse$PRK_SUFFIX    10 10 5
        $PRK_LAUNCHER -np $PRK_MPI_PROCS -nfg $PRK_FGMPI_THREADS $PRK_TARGET_PATH/dgemm$PRK_SUFFIX     10 1024 32 1
        $PRK_LAUNCHER -np $PRK_MPI_PROCS -nfg $PRK_FGMPI_THREADS $PRK_TARGET_PATH/random$PRK_SUFFIX    32 20
        $PRK_LAUNCHER -np $PRK_MPI_PROCS -nfg $PRK_FGMPI_THREADS $PRK_TARGET_PATH/global$PRK_SUFFIX    10 16384
        ;;
    allgrappa)
        echo "Grappa"
        ########################
        #. $TRAVIS_ROOT/grappa/bin/settings.sh
        export GRAPPA_PREFIX=$TRAVIS_ROOT/grappa
        export SCRIPT_PATH=$TRAVIS_ROOT/grappa/bin
        ########################
        echo "GRAPPATOP=$TRAVIS_ROOT/grappa" >> common/make.defs
        make $PRK_TARGET
        export PRK_TARGET_PATH=Cxx11
        export PRK_SUFFIX=-grappa
        export PRK_MPI_PROCS=2
        export PRK_LAUNCHER=$MPI_ROOT/bin/mpirun
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/p2p$PRK_SUFFIX       10 1024 1024
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/stencil$PRK_SUFFIX   10 1000
        $PRK_LAUNCHER -n $PRK_MPI_PROCS $PRK_TARGET_PATH/transpose$PRK_SUFFIX 10 1024 32
        ;;
    allchapel)
        echo "Nothing to do yet"
        ;;
    allhpx3)
        echo "Nothing to do yet"
        ;;
    allhpx5)
        echo "Nothing to do yet"
        ;;
esac
