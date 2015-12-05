echo cleaning krylov...
cd ./krylov
rm -f $1 *.o *.a *~
echo cleaning multivector...
cd ../multivector
rm -f $1 *.o *.a *~
echo cleaning parcsr_ls...
cd ../parcsr_ls
rm -f $1 *.o *.a *~
echo cleaning struct_ls...
cd ../struct_ls
rm -f $1 *.o *.a *~
echo cleaning sstruct_ls...
cd ../sstruct_ls
rm -f $1 *.o *.a *~
echo cleaning utilities...
cd ../utilities
rm -f $1 *.o *.a *~
echo cleaning test...
cd ../test
rm -f $1 *.o *.a ij_es struct_es sstruct_es *.exe* vectors.* val*.txt res*.txt *.0* *~
echo cleaning TEST_ij_es...
cd ./TEST_ij_es
rm -f $1 ij_es.exe ij_es *.jobs *.out.* *.0* vectors.* val*.txt res*.txt *~
echo cleaning TEST_struct_es...
cd ../TEST_struct_es
rm -f $1 struct_es.exe struct_es *.jobs *.out.* *.0* vectors.* val*.txt res*.txt *~
echo cleaning TEST_sstruct_es...
cd ../TEST_sstruct_es
rm -f $1 sstruct_es.exe sstruct_es *.jobs *.out.* *.0* vectors.* val*.txt res*.txt *~
cd ../..
rm -f $1 *~
echo removing libraries...
rm -f -r $1 ./hypre
