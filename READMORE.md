# enfield代码结构

*本文档为enfield代码阅读过程中记录的内容，主要针对QAllocator相关部分，如有不足和缺失请及时补充*
enfield [Github地址](https://github.com/ysiraichi/enfield)

**目录**
- [enfield代码结构](#enfield%e4%bb%a3%e7%a0%81%e7%bb%93%e6%9e%84)
  - [目录和主要文件说明](#%e7%9b%ae%e5%bd%95%e5%92%8c%e4%b8%bb%e8%a6%81%e6%96%87%e4%bb%b6%e8%af%b4%e6%98%8e)
  - [QAllocation过程](#qallocation%e8%bf%87%e7%a8%8b)
  - [如何添加一个QAllocator](#%e5%a6%82%e4%bd%95%e6%b7%bb%e5%8a%a0%e4%b8%80%e4%b8%aaqallocator)
    - [提示](#%e6%8f%90%e7%a4%ba)
  - [主要类和函数](#%e4%b8%bb%e8%a6%81%e7%b1%bb%e5%92%8c%e5%87%bd%e6%95%b0)
    - [QModule](#qmodule)
    - [Pass](#pass)
    - [Init](#init)
    - [InitializeAllQbitAllocators](#initializeallqbitallocators)
    - [ParseArguments](#parsearguments)
    - [Opt](#opt)
    - [Compile](#compile)
    - [QbitAllocator](#qbitallocator)
  - [TODO](#todo)

## 目录和主要文件说明

- test: 测试代码
- include: 头文件所在位置，各种类的定义都在这里
- lib: 各类的成员函数实现所在位置
- tools: 工具和入口函数

## QAllocation过程

- 在编译目标`efd`的入口（见[`tools/Enfield.cpp`](tools/Enfield.cpp)），[读取参数](#ParseArguments)然后调用[Compile](#Compile)函数
- (#Compile)函数创建了一个[QbitAllocator](#QbitAllocator)（见[CreateQbitAllocator](#CreateQbitAllocator)）
- 设置门的权重（见[setGateWeightMap](#TODO)）
- 执行`QbitAllocator`,调用其[run](#TODO)方法
- `QbitAllocator::run`的过程中调用[allocate](#TODO)成员函数

## 如何添加一个QAllocator

*可能不完整，请继续补充*

- 在[`Allocators.def`](include/enfield/Transform/Allocators/Allocators.def)中添加声明
- 在[`include/enfield/Transform/Allocators/`](include/enfield/Transform/Allocators/)和[对应位置](lib/Transform/Allocators/)实现一个[QbitAllocator](#QbitAllocator)的子类
- 在[`tests/`](tests/)路径下添加测试代码
- 在各处的CMakeLists（[这里](lib/Transform/Allocators/CMakeLists.txt)和[这里](tests/CMakeLists.txt)）添加文件

### 提示

实际上QbitAllocator的核心操作就是其`allocate`方法，我们直接重写这个方法就可以了。
CODAR和SABRE都是从给定初始映射开始求解和remapping的，
建议参照[SABRE的实现](lib/Transform/Allocators/SabreQAllocator.cpp)。
（我觉得可以直接作为`SabreQAllocator`的子类重写`allocateWithInitialMapping`方法，再调整一下其他位置就可以了）

## 主要类和函数

### QModule

定义位于[`include/enfield/Transform/QModule.h`](include/enfield/Transform/QModule.h)

表示一个QASM模块，提供各种对QASM进行操作的函数

### Pass

定义位于[`include/enfield/Transform/Pass.h`](include/enfield/Transform/Pass.h)

实现一个Pass

### Init

定义位于[`lib/Transform/Driver.cpp`](lib/Transform/Driver.cpp)

执行各种初始化函数，并[解析命令行参数](#ParseArguments)

### InitializeAllQbitAllocators

定义位于[`lib/Transform/Allocators/Allocators.cpp`](lib/Transform/Allocators/Allocators.cpp)

根据[`Allocators.def`](include/enfield/Transform/Allocators/Allocators.def)注册各个QbitAllocator

### ParseArguments

定义位于[`lib/Support/CommandLine.cpp`](lib/Support/CommandLine.cpp)

解析参数列表。如果参数列表中含有某个参数，
则调用对应[Opt](#Opt)类的`parse`方法解析参数，
对应[Opt](#Opt)类中`mIsParsed`字段被置为`true`。

### Opt

位于[`include/enfield/Support/CommandLine.h`](include/enfield/Support/CommandLine.h)

实现命令行参数读取。

重要成员函数：

- `bool isParsed()`: 如果命令行参数列表包含此参数，则返回`true`，见[ParseArguments](#ParseArguments)
- `T getVal()`: 返回参数的值

### Compile

位于[`lib/Transform/Driver.cpp`](lib/Transform/Driver.cpp)

根据设置（`settings`参数）生成各种[Pass](#Pass)并执行

### QbitAllocator

位于[`include/enfield/Transform/Allocators/QbitAllocator.h`](include/enfield/Transform/Allocators/QbitAllocator.h)

[Pass](#Pass)的派生类，用于实现Allocation（Mapping）




## TODO

请继续补充