"""
V13-Vision + k230_example UART
视觉部分来自 v13_bidirectional（双向方向选择 + 单应矩阵映射）
串口发送逻辑来自 k230_example（帧头 0x55 0xAA，cx/cy 高低字节拆分，帧尾 0xFA）
"""
import time, gc, math
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

CANNY_LO=50; CANNY_HI=150; EPSILON=0.08; AREA_MIN=0.001; ANGLE_COS=0.6; BLUR_SIZE=5

PAPER_W_PORT=21.0; PAPER_H_PORT=29.7
PAPER_W_LAND=29.7; PAPER_H_LAND=21.0
TAPE_W_CM=1.8

DST_W_PORT=210; DST_H_PORT=297
DST_W_LAND=297; DST_H_LAND=210
R_PX=60; N_SAMPLE=72

SAMPLE_OFFSET=6; SAMPLE_COUNT=5
SMOOTH_ALPHA=0.35; TYPE_HIST_LEN=3

# ★ 手动强制横竖 (None=自动)
FORCE_ORIENT = None   # True=横放, False=竖放, None=自动

sensor=Sensor(id=2,width=1280,height=960,fps=90)
sensor.reset();sensor.set_framesize(width=W,height=H);sensor.set_pixformat(Sensor.RGB888)
try:g=k_sensor_gain();g.gain[0]=20;sensor.again(g)
except:pass
Display.init(Display.ST7701,width=W,height=H,to_ide=True,quality=50)
MediaManager.init();sensor.run();time.sleep_ms(200)
clock=time.clock()
print("V13-Vision + k230 UART")

# ===== 单应矩阵 =====

def solve_homography(src,dst):
    M=[[0.0]*9 for _ in range(8)]
    for i in range(4):
        x,y=float(src[i][0]),float(src[i][1]);u,v=float(dst[i][0]),float(dst[i][1])
        r0=2*i;r1=2*i+1
        M[r0][0]=x;M[r0][1]=y;M[r0][2]=1.0;M[r0][3]=0;M[r0][4]=0;M[r0][5]=0
        M[r0][6]=-x*u;M[r0][7]=-y*u;M[r0][8]=u
        M[r1][0]=0;M[r1][1]=0;M[r1][2]=0;M[r1][3]=x;M[r1][4]=y;M[r1][5]=1.0
        M[r1][6]=-x*v;M[r1][7]=-y*v;M[r1][8]=v
    for col in range(8):
        pivot=col
        for row in range(col+1,8):
            if abs(M[row][col])>abs(M[pivot][col]):pivot=row
        if abs(M[pivot][col])<0.000001:return None
        if pivot!=col:M[col],M[pivot]=M[pivot],M[col]
        for row in range(8):
            if row!=col:
                f=M[row][col]/M[col][col]
                for j in range(9):M[row][j]-=f*M[col][j]
    h=[0.0]*8
    for i in range(8):h[i]=M[i][8]/M[i][i]
    return h

def map_point(h,x,y):
    ww=h[6]*x+h[7]*y+1.0
    if abs(ww)<0.000001:return 0.0,0.0
    return(h[0]*x+h[1]*y+h[2])/ww,(h[3]*x+h[4]*y+h[5])/ww

# ===== 角点 =====

def sort_corners(pts):
    sums=[p[0]+p[1] for p in pts]
    i_tl=sums.index(min(sums));i_br=sums.index(max(sums))
    rem=[i for i in range(4)if i not in(i_tl,i_br)]
    if pts[rem[0]][0]>pts[rem[1]][0]:i_tr,i_bl=rem[0],rem[1]
    else:i_tr,i_bl=rem[1],rem[0]
    return[pts[i_tl],pts[i_tr],pts[i_br],pts[i_bl]]

def is_convex_quad(pts):
    def cross(o,a,b):return(a[0]-o[0])*(b[1]-o[1])-(a[1]-o[1])*(b[0]-o[0])
    signs=[cross(pts[i],pts[(i+1)%4],pts[(i+2)%4])for i in range(4)]
    return not(any(s>0 for s in signs)and any(s<0 for s in signs))

def quad_area(pts):
    return 0.5*abs(pts[0][0]*pts[1][1]+pts[1][0]*pts[2][1]+pts[2][0]*pts[3][1]+pts[3][0]*pts[0][1]
                   -pts[1][0]*pts[0][1]-pts[2][0]*pts[1][1]-pts[3][0]*pts[2][1]-pts[0][0]*pts[3][1])

# ===== 双向计算 ★ V13核心 =====

def get_dims(landscape,frame_type):
    """返回 (pap_w, pap_h, dst_w, dst_h)"""
    if landscape:w=PAPER_W_LAND;h=PAPER_H_LAND;dw=DST_W_LAND;dh=DST_H_LAND
    else:w=PAPER_W_PORT;h=PAPER_H_PORT;dw=DST_W_PORT;dh=DST_H_PORT
    if frame_type=="INNER":
        w-=2*TAPE_W_CM;h-=2*TAPE_W_CM
        dw=int(dw*w/(w+2*TAPE_W_CM));dh=int(dh*h/(h+2*TAPE_W_CM))
    return w,h,dw,dh

def compute_circle_pts(src_pts,landscape,frame_type):
    w,h,dw,dh=get_dims(landscape,frame_type)
    dst_pts=[(0,0),(dw,0),(dw,dh),(0,dh)]
    h_inv=solve_homography(dst_pts,src_pts)
    if h_inv is None:return None,0

    cx_d=dw//2;cy_d=dh//2
    pts=[]
    xs=[];ys=[]
    for i in range(N_SAMPLE):
        ang=2.0*math.pi*i/N_SAMPLE
        px,py=map_point(h_inv,cx_d+R_PX*math.cos(ang),cy_d+R_PX*math.sin(ang))
        pts.append((int(px),int(py)))
        xs.append(px);ys.append(py)

    if not xs:return None,0
    ar=(max(xs)-min(xs))/max((max(ys)-min(ys)),1.0)
    return pts,ar

def pick_orientation(src_pts,frame_type):
    if FORCE_ORIENT is not None:
        return FORCE_ORIENT
    pts_port,ar_port=compute_circle_pts(src_pts,False,frame_type)
    pts_land,ar_land=compute_circle_pts(src_pts,True,frame_type)
    if pts_port is None and pts_land is None:return False
    if pts_port is None:return True
    if pts_land is None:return False
    d_port=abs(math.log(max(ar_port,0.01)))
    d_land=abs(math.log(max(ar_land,0.01)))
    return d_land<d_port

def get_best_pts(src_pts,frame_type):
    landscape=pick_orientation(src_pts,frame_type)
    pts,_=compute_circle_pts(src_pts,landscape,frame_type)
    return pts,landscape

# ===== 绘制 + 分类 =====

def draw_quad(img,r,color):
    pts=[(r[4],r[5]),(r[6],r[7]),(r[8],r[9]),(r[10],r[11])]
    for i in range(4):
        x1,y1=pts[i];x2,y2=pts[(i+1)%4]
        img.draw_line(x1,y1,x2,y2,color=color)

def diagonal_center(r):
    x1,y1=r[4],r[5];x2,y2=r[8],r[9];x3,y3=r[6],r[7];x4,y4=r[10],r[11]
    d=(x1-x2)*(y3-y4)-(y1-y2)*(x3-x4)
    if abs(d)<0.001:return(x1+x2+x3+x4)//4,(y1+y2+y3+y4)//4
    t=float((x1-x3)*(y3-y4)-(y1-y3)*(x3-x4))/d
    return int(x1+t*(x2-x1)),int(y1+t*(y2-y1))

def sample_gray(img_np,x,y):
    h,w=img_np.shape[0],img_np.shape[1]
    x=max(0,min(w-1,int(x)));y=max(0,min(h-1,int(y)))
    return int(img_np[y,x][0]+img_np[y,x][1]+img_np[y,x][2])//3

def sample_side(img_np,p1,p2,cx,cy,n,off):
    iv,ov=[],[]
    for k in range(n):
        t=k/(n-1)if n>1 else 0.5
        px=p1[0]+t*(p2[0]-p1[0]);py=p1[1]+t*(p2[1]-p1[1])
        dx=cx-px;dy=cy-py;L=(dx*dx+dy*dy)**0.5
        if L<0.5:continue
        nx=dx/L;ny=dy/L
        iv.append(sample_gray(img_np,px+nx*off,py+ny*off))
        ov.append(sample_gray(img_np,px-nx*off,py-ny*off))
    if not iv:return 128,128
    return sum(iv)//len(iv),sum(ov)//len(ov)

def classify_frame(img_np,rect):
    cx,cy=diagonal_center(rect)
    edges=[((rect[4],rect[5]),(rect[6],rect[7])),((rect[6],rect[7]),(rect[8],rect[9])),
           ((rect[8],rect[9]),(rect[10],rect[11])),((rect[10],rect[11]),(rect[4],rect[5]))]
    vi=vo=0
    for p1,p2 in edges:
        ai,ao=sample_side(img_np,p1,p2,cx,cy,SAMPLE_COUNT,SAMPLE_OFFSET)
        if ai<ao-15:vo+=1
        elif ao<ai-15:vi+=1
    if vo>vi:return"OUTER"
    if vi>vo:return"INNER"
    return"?"

# ===== 主循环 =====

frame_type_smooth="?"
type_history=[]
cached_pts=[]

while True:
    clock.tick()
    img=sensor.snapshot();np_img=img.to_numpy_ref()

    rects=cv_lite.rgb888_find_rectangles_with_corners(
        IMG,np_img,CANNY_LO,CANNY_HI,EPSILON,AREA_MIN,ANGLE_COS,BLUR_SIZE)
    n=len(rects)

    best=None;ba=0
    for i in range(n):
        a=rects[i][2]*rects[i][3]
        if a>ba:ba=a;best=rects[i]

    if best:
        frame_type_raw=classify_frame(np_img,best)
        type_history.append(frame_type_raw)
        if len(type_history)>TYPE_HIST_LEN:type_history.pop(0)
        if len(type_history)>=TYPE_HIST_LEN and all(t==type_history[0]for t in type_history):
            frame_type_smooth=type_history[0]
        if frame_type_smooth=="?":frame_type_smooth=frame_type_raw
        frame_type=frame_type_smooth

        raw_corners=[(best[4],best[5]),(best[6],best[7]),(best[8],best[9]),(best[10],best[11])]
        src_pts=sort_corners(raw_corners)
        ok=is_convex_quad(src_pts)and quad_area(src_pts)>100

        if ok:
            pts,landscape=get_best_pts(src_pts,frame_type)
            if pts is not None:cached_pts=pts
            else:pts=cached_pts;landscape=False
        else:
            pts=cached_pts;landscape=False

        if frame_type=="OUTER":quad_color=(0,255,255)
        elif frame_type=="INNER":quad_color=(255,165,0)
        else:quad_color=(128,128,128)

        draw_quad(img,best,color=quad_color)
        for j in range(4):
            img.draw_cross(best[4+j*2],best[4+j*2+1],color=(255,255,0),size=4,thickness=1)
        img.draw_line(best[4],best[5],best[8],best[9],color=(255,0,255))
        img.draw_line(best[6],best[7],best[10],best[11],color=(255,0,255))
        cx,cy=diagonal_center(best)
        img.draw_cross(cx,cy,color=(0,255,0),size=15,thickness=3)

        if len(pts)>=2:
            for i in range(len(pts)):
                x1,y1=pts[i];x2,y2=pts[(i+1)%len(pts)]
                img.draw_line(x1,y1,x2,y2,color=(0,255,0),thickness=2)

        ori="LAND"if landscape else"PORT"
        if FORCE_ORIENT is not None:ori+="!"
        info="({},{}) pts={} [{} {}]".format(cx,cy,len(pts),frame_type,ori)

        # 串口发送数据 (k230_example 协议)
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
        except Exception as e:
            print("UART error:", e)
    else:
        frame_type="?"
        pts=cached_pts
        if len(pts)>=2:
            for i in range(len(pts)):
                x1,y1=pts[i];x2,y2=pts[(i+1)%len(pts)]
                img.draw_line(x1,y1,x2,y2,color=(0,100,0),thickness=1)
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
    img.draw_string_advanced(10,5,22,f"V13t FPS:{fps:.1f} [{frame_type}]",color=(0,255,0))
    if best:
        img.draw_string_advanced(10,35,22,f"N={N_SAMPLE} r={R_PX}px",color=(0,255,0))
    Display.show_image(img)
    print("V13t FPS:{:.1f} n:{} {}".format(fps,n,info))
    gc.collect()
