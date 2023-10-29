# pressure probe
一个自制的低成本压力热床，通过检测喷嘴接触热床产生的压力来达到调平和网床测量的功能
## 硬件
<img width="998" alt="image" src="https://github.com/xcqq/pressure_probe/assets/18913791/6f5f62ae-e65e-4be7-ad42-4da2d2c5180a">

### 原理图以及PCB
原理图和PCB开源在立创开源平台（还在审核）：https://oshwhub.com/froyo94/pressure_probe

### 基本信息
* 主控：stm32g030f6p6 
* ADC：CS1237 
* 传感器：全桥压力传感器, 5kg量程，1mv/v输出，这里附上购买连接: https://item.taobao.com/item.htm?spm=a1z09.2.0.0.482e2e8dFrEjWm&id=576798890607&_u=tk12k303eda
* 供电： SPX3819理论支持5-12V供电输入
* 成本：大约在5RMB以内
* 精度：重复探测精度大约在0.005以内，取决于z速度，触发压力，以及热床本身的震动
  <img width="652" alt="image" src="https://github.com/xcqq/pressure_probe/assets/18913791/26d45fe4-542a-4319-8f34-bd2f1eb2763b">

* 触发压力：默认触发压力为250g，实际探测压力大约为300g
  <img width="675" alt="image" src="https://github.com/xcqq/pressure_probe/assets/18913791/199333fa-c746-4af6-bebd-b0ae25f263da">
  
### 接口定义
接口定义如下，也可以参考PCB上的丝印标注
* 输入接口: AINP AINN VCC GND
* 输出接口: VCC GND OUT
* 调试串口: RX TX 3.3V GND
* 下载接口: DIO CLK 3.3V GND
  
## 功能
基本功能当然就是调平了。调平的思路其实也很简单，通过采集滑动窗口内热床压力的均值，并和当前压力值做比较。如果当前压力值减去均值大于设定的阈值则认为喷嘴压下，这时候就会触发输出。
相比起类电子秤的方案主要有以下几个改进：
* 高采样率(1240HZ)：可以保证在z快速移动的情况下也有较高的精度
* 边沿检测代替固定的压力值检测：避免温漂的影响，也不用手动去校准热床重量
* 较低的触发压力：代码默认的触发压力仅为250g


## 如何使用
请注意如果使用了不同的压力传感器，请一定要参照后文修改配置
1. 编译代码：代码是使用arduino框架编写，需要选择generic stm32g030f6主控类型
2. 下载固件：编译完成后，使用stlink通过swd接口下载固件
3. 连线：连接压力传感器和限位输出
4. 检查是否工作正常：轻敲热床，观察probe指示灯是否亮起
5. 可以愉快的玩耍啦

## 一些装机图片
![c3ea11030f5fa4d87adeda13cd47c202_720](https://github.com/xcqq/pressure_probe/assets/18913791/559d804d-cd45-465d-b77e-7d6e3d37d4a9)
![015c85ffd0bfe8d8782fd6e17e1499c8_720](https://github.com/xcqq/pressure_probe/assets/18913791/52f563c8-a1ab-4008-8f2b-c1b0740528e2)
![6f43cf3bd518bb57f6b834be3c2766a2](https://github.com/xcqq/pressure_probe/assets/18913791/1013c4d4-837a-40c3-bbe7-a575c57c16b9)


### 修改配置
代码中留下了一部分预定义的参数供大家修改

#### 压力传感器相关
根据购买的不同传感器需要填入对应的参数，参数不正确会极大的影响效果，甚至无法触发

```c
// pressure sensor configure
#define SENSOR_MAX_WEIGHT 5000 //传感器量程，单位g
#define SENSOR_MAX_VOLT 1 // 传感器输出灵敏度，单位mv
#define SENSOR_REVERSE 1 //是否反向，根据安装方式不同可能输出的压力值是负值
```
#### 边沿检测相关

```c
// edge detection configure
#define WINDOW_SIZE 500 //滑动窗口大小
#define THRESHOLD_STATIC 250 // 触发压力值，单位g
#define EDGE_CONTINUOUS 1 // 连续几次检测超过阈值才认为喷嘴下压，可能可以改善重复定位精度
```
## TODO
* 压力检测部分改到喷嘴上实现Loadcell（那就不叫压力热床了？）
* 动态压力阈值（大概做不出来哎）
