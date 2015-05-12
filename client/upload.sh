ninja -C ../build
rsync -v ../build/app *.py users:mbuild/
