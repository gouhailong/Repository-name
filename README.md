# STM32-F103-MPU6050-振动异常检测终端
## 项目简介
基于 STM32F103C8T6 + MPU6050 + FreeRTOS 实现工业设备振动在线监测，200Hz高频采样，自带自学习基线、状态机报警逻辑，配套Python上位机可视化。

## 核心功能
1. 200Hz三轴加速度采样 + 滑动窗口时域特征提取
2. RMS有效值、峰值、峰值因子融合评分算法，自学习基线阈值
3. 报警保持/恢复保持双机制，降低误报、漏报概率
4. FreeRTOS多任务解耦：采样、滤波检测、串口通信、独立看门狗
5. IWDG硬件看门狗保障设备稳定运行
6. UART输出JSON格式日志，Python/Tkinter上位机实时绘制RMS/P2P/Score曲线

## 硬件平台
- 主控：STM32F103C8T6
- 传感器：MPU6050 六轴加速度陀螺仪模块

## 开发环境
- STM32CubeMX 代码配置
- FreeRTOS V9 实时操作系统
- 上位机：Python3 + Tkinter + Matplotlib