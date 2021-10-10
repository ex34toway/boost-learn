# Boost

## Boost 编译

### 静态编译

```cmd
 .\b2.exe --with-thread --stagedir=".\stage_x64" address-model=64 link=static variant=debug
 .\b2.exe --with-thread --stagedir=".\stage_x64" address-model=64 runtime-link=static variant=debug
```

## 项目编译

```cmd
mkdir build
cd build
cmake -DBOOST_ROOT=D:\boost_1_70_0 -DBOOST_LIBRARYDIR=D:\boost_1_70_0\stage_x64\lib ..
```
