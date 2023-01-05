# 基于树莓派的CAN信号解析系统

## 项目概述

一套基于树莓派的CAN信号解析设备。使用便利，提供DBC文件并且连接上CAN总线即可对CAN信号进行解析，获取自己需要的信息。取代传统的CANoe软件及vector设备。读取信息方式更灵活，支持树莓派桌面，手机APP（Android）、及网页界面。提供数据api接口方便公司各部门调用，数据统一存储到数据库中。

## 系统框架

### 前端

- 桌面应用：PYQT以及Tkinter（基于python的桌面应用包）
- 手机app：Android
- 网页界面：html+css

### 后端

基于django的web后台（python） 实现HTTP api接口，MQTT api接口

### 数据库

Mysql    SQLite（Android） 

数据库部署在服务器上，若数据量庞大需转到公司的服务器中

### 硬件

emmc版本树莓派为主控板，can线，hdmi接口屏幕（触摸屏）ps:预留所有树莓派能提供的外设接口

现仓库版本是基于STM32的开发

### 服务器

暂定，可部署到云服务器上。

## 系统设计图

暂时无法在飞书文档外展示此内容

## 需求分析

- 传统can读取方式繁琐。
- 数据查看不灵活等问题。
- 数据回放繁琐无法实时监控等问题。
- 数据解析后无法存储等问题。

## 设备功能

- 仿can卡功能：读取CAN总线的报文信息并可对CAN信号进行解析，解析是以KEY-VALUE形式。
- 数据多通道阅读功能：对所解析的can信息进行筛选，对所需已解析信号采用多方式呈现，Android，网页，上位机桌面应用。
- 数据上报功能：对所解析的can信息进行系统上报
- 数据管理功能：将所解析的键值对存储至服务器的数据库中，后期将会提供接口对数据的分权限访问读取。

## 设备成本

- EMMC版树莓派 ¥606*.00*（https://www.waveshare.net/shop/CM4-IO-WIRELESS-BASE-Acce-A.htm）
- 13.3寸电容屏 ¥873.65（https://www.waveshare.net/shop/13.3inch-HDMI-LCD-H-with-Holder.htm）
- 外壳设计与加工 暂时未定
- can信号线成本忽略不计

## 可行性分析

### 经济可行性

- 取代传统的vector cancase设备，一套两通道的设备十几万，设备落地能大大降低设备购买成本（只用设备调试读取can信息及发送can信息，可代替）
- 硬件方面主控树莓派硬件成本是一千以内 外壳设计，can口排线电源设备可忽略不计，软件方面自行研发，需搭建后端前端，以及后期测试有时间精力成本。

### 技术可行性

- 主要的解析功能已经实现，项目落地的可行性有保证，前端设计若是简易版开发速度很快，后端搭建包含学习及实践需几周开发时间，简易版落地需两个月。
- 所需硬件由lto部门提供。
- 本项目是解耦的软件方面，硬件方面以及服务器后端方面都可以并行开发。

## 开发周期

### 一期阶段

- 后端平台搭建成型，http接口开通能正常post，get请求。（2023/1/8）
- 数据库建立成型，先以电池设备为例建立设备信息表，后端能正进行增删改查。(2023/1/8)
- 硬件搭建完成。(2022/12/30)

### 二期阶段

- 硬件端改进升级，实现网页与手机端的界面设计。（2023/2/18）
- 软件硬件性能测试。（2023/3/1）
- 交付电池检测组。（2023/3/15）