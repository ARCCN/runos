# Print glog messages
export DYLD_LIBRARY_PATH=$PWD/prefix/lib:$DYLD_LIBRARY_PATH # OS X
export LD_LIBRARY_PATH=$PWD/prefix/lib:$LD_LIBRARY_PATH   # Linux
export GLOG_logtostderr=1
export GLOG_colorlogtostderr=1
