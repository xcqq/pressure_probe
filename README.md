
# pressure probe
一个自制的低成本压力热床，通过检测喷嘴接触热床产生的压力来达到调平和网床测量的功能
欢迎各位品尝，请务必仔细阅读一下说明，以防止错误使用导致的机器损坏！！！！！！
## 硬件
<img width="1027" alt="image" src="https://github.com/xcqq/pressure_probe/assets/18913791/eb21c66a-32b5-4aa8-b606-b6c9d4208c3a">

### 原理图以及PCB
原理图和PCB开源在立创开源平台：https://oshwhub.com/froyo94/pressure_probe

### 基本信息
* 主控：stm32g030f6p6 
* ADC：CS1237 
* 传感器：全桥压力传感器，理论上不同量程的传感器都可使用，需要在代码中进行配置
  我是用的传感器是50kg量程，1mv/v输出，这里附上购买连接（仅供参考）: https://item.taobao.com/item.htm?spm=a1z09.2.0.0.76782e8d35evLY&id=578671630098&_u=6k12k30c479
  <img width="268" alt="image" src="https://github.com/xcqq/pressure_probe/assets/18913791/f7dc5faa-36ca-4ee9-be5b-30a58e19c8da">

* 供电： SPX3819理论支持5-12V供电输入，输入前经过RC滤波
* 成本：大约在5RMB以内
* 精度：重复探测精度大约在0.01以内，取决于z速度，触发压力，以及热床本身的震动
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

## 如何固定压力传感器
这需要大家自由发挥，这里给出三叉戟上的安装图片以供参考：
![IMG_20231029_174314](https://github.com/xcqq/pressure_probe/assets/18913791/c1f1e0ac-f15d-4138-b33c-06bd839e66e6)
![IMG_20231029_174336](https://github.com/xcqq/pressure_probe/assets/18913791/0429d0b7-6b1c-4bff-b449-ab16e4a7ded2)

## 如何使用
请注意如果使用了不同的压力传感器，请一定要参照后文修改配置
1. 编译代码：代码是使用arduino框架编写，需要选择generic stm32g030f6主控类型
2. 下载固件：编译完成后，使用stlink通过swd接口下载固件
3. 连线：连接压力传感器和限位输出，多个全桥压力传感器可直接并联
4. 连接接地线：将压力传感器的地和压力传感器的金属外壳相连，这可以极大程度的上减小热床耦合进来的干扰
5. 检查传感器极性：按压或者抬起热床，观察probe指示灯是否亮起，若抬起热床时probe灯亮起，需要反装压力传感器，或者更改代码中SENSOR_REVERSE的值
6. 配置探针的xy偏移：修改配置文件中的probe xy offset为0
7. 进行首次归零测试：建议这里使用废旧底板，或者不易刮花的底板，进行一个z归零测试
8. 修改触发阈值：如果遇到误触发可以通过按钮修改修改触发阈值，按一下增加50g，最小100g,最大400g。按一次会有白灯闪烁一次提示，快闪三次表示超过最大值，阈值重新回到最小值。
9. 进行网床测试
10. 首层打印测试，修改z offset到合适的值
11. 可以愉快的玩耍啦

## 效果参考
![IMG_20231105_152553](https://github.com/xcqq/pressure_probe/assets/18913791/e23f86c9-e465-4786-b6e9-7330f5d00493)

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
// filter configure
#define FILTER_WINDOW_SIZE 20 //中值滤波滑动窗口大小

// edge detection configure
#define WINDOW_SIZE 500 //压力均值滑动窗口大小
#define THRESHOLD_MIN 100 // 最小触发阈值
#define THRESHOLD_MAX 400 // 最大触发阈值
#define THRESHOLD_STEP 50 // 按钮增压步进值
```
## TODO
- [x] 通过按钮配置压力阈值
- [ ] 压力检测部分改到喷嘴上实现Loadcell（那就不叫压力热床了？）
- [ ] 动态压力阈值（废弃）
