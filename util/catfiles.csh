#!/usr/local/bin/tcsh
# concatenate per-CPU output files
foreach file (*.0)
  # strip off .0 at the end of $file
  set new=`echo $file | sed -e 's/\.0$//'`
  set head=`echo $new | sed -e 's/\.chkpt$/.head/'`
  touch $head
  cat $head $new.* > $new
  rm -f $new.*
  rm -f $head
end
