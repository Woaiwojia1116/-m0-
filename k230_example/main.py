"""
V6 - PnP测距 + 快速检测 (混合模式)

每帧: find_rectangles_with_corners 快速检测(FPS30+) → 画图
每5帧: 插入一次PnP测距 → 更新精确距离
FPS: 19
"""
import time, gc
from media.sensor import *
from media.display import *
from media.media import *
from machine import UART
import cv_lite

W, H = 640, 480; IMG = [H, W]

# 串口初始化 (115200波特率 GPIO9=TX, GPIO10=RX)
uart = UART(1, baudrate=115200, tx=9, rx=10)
# 降低串口发送频率
count = 0

# 快速检测参数
CLO=50; CHI=150; EPS=0.08; AMIN=0.001; ACOS=0.6; BLR=5

# PnP相机参数
CAM=[
    1199.1495245491617,0.0,670.4213222250077,
    0.0,1200.6879329729338,475.11122810359876,
    0.0,0.0,1.0
]
DIST=[-0.0331624688297151,0.04145795463721313,
      0.0016964134879770974,0.0008834973490863845,
      -0.05414957483933673]
DL=5; OW=21.0; OH=29.7; PI=5  # PnP测距间隔帧数

sensor=Sensor(id=2,width=1280,height=960,fps=90)
sensor.reset()
sensor.set_framesize(w=W,h=H,chn=CAM_CHN_ID_0)
sensor.set_pixformat(Sensor.RGB888)
try:
    g=k_sensor_gain();g.gain[0]=20;sensor.again(g)
except:pass

Display.init(Display.ST7701,width=W,height=H,to_ide=True,quality=50)
MediaManager.init();sensor.run();time.sleep_ms(200)

clock=time.clock()
last_d=-1; fc=0
print("V6 - Hybrid PnP")

def draw_quad(img,r,color):
    # 绘制四边形
    pts=[(r[4],r[5]),(r[6],r[7]),(r[8],r[9]),(r[10],r[11])]
    for i in range(4):
        x1,y1=pts[i];x2,y2=pts[(i+1)%4]
        img.draw_line(x1,y1,x2,y2,color=color)

def dcenter(r):
    # 计算矩形中心点
    x1,y1=r[4],r[5];x2,y2=r[8],r[9]
    x3,y3=r[6],r[7];x4,y4=r[10],r[11]
    d=(x1-x2)*(y3-y4)-(y1-y2)*(x3-x4)
    if abs(d)<0.001:return(x1+x2+x3+x4)//4,(y1+y2+y3+y4)//4
    t=float((x1-x3)*(y3-y4)-(y1-y3)*(x3-x4))/d
    return int(x1+t*(x2-x1)),int(y1+t*(y2-y1))

while True:
    clock.tick();fc+=1
    img=sensor.snapshot();np_img=img.to_numpy_ref()

    # 每帧: 快速检测矩形
    rects=cv_lite.rgb888_find_rectangles_with_corners(
        IMG,np_img,CLO,CHI,EPS,AMIN,ACOS,BLR)
    n=len(rects)
    best=None;ba=0
    for i in range(n):
        a=rects[i][2]*rects[i][3]
        if a>ba:ba=a;best=rects[i]

    # 每5帧执行一次PnP测距
    if best and fc%PI==0:
        try:
            r=cv_lite.rgb888_pnp_distance_from_corners(
                IMG,np_img,CAM,DIST,DL,OW,OH)
            if r[0]>0:last_d=r[0]/2
        except:pass

    # 绘制 + 串口发送中心坐标
    if best:
        draw_quad(img,best,color=(0,255,255))
        for j in range(4):
            img.draw_cross(best[4+j*2],best[4+j*2+1],color=(255,255,0),size=4,thickness=1)
        cx,cy=dcenter(best)
        img.draw_cross(cx,cy,color=(0,255,0),size=15,thickness=3)
        if last_d>0:
            img.draw_string_advanced(10,35,22,f"d={last_d:.1f}cm",color=(0,255,0))
            info=f"d={last_d:.1f} ({cx},{cy})"
        else:info=f"({cx},{cy})"

        # 串口发送数据
        try:
            count += 1
            if count > 5:
                # 拆分 cx 为高8位和低8位
                cx_low = cx & 0xFF
                cx_high = (cx >> 8) & 0xFF
                # 拆分 cy 为高8位和低8位
                cy_low = cy & 0xFF
                cy_high = (cy >> 8) & 0xFF

                # 打包发送: 帧头(2字节) + cx高低(2字节) + cy高低(2字节) + 帧尾(1字节)
                data = bytearray([0x55, 0xaa, cx_high, cx_low,  cy_high, cy_low, 0xfa])
                uart.write(data)
                count = 0
#                print(f"Sent: cx={cx}(0x{cx:04X}), cy={cy}(0x{cy:04X})")  # 调试信息
        except Exception as e:
            print("UART error:", e)
    else:
        info="none"
        # 识别不到矩形时发送 x=65535, y=65535
        try:
            count += 1
            if count > 5:
                data = bytearray([0x55, 0xaa, 0xFF, 0xFF, 0xFF, 0xFF, 0xfa])
                uart.write(data)
                count = 0
        except Exception as e:
            print("UART error:", e)

    fps=clock.fps()
    img.draw_string_advanced(10,5,22,f"V6 FPS:{fps:.1f}",color=(0,255,0))
    Display.show_image(img)
    print(f"V6 FPS:{fps:.1f} n:{n} {info}")
    gc.collect()
