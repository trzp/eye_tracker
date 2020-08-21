#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Time    : 2020/8/20 23:54
# @Version : 1.0
# @File    : py4c.py
# @Author  : Jingsheng Tang
# @Version : 1.0
# @Contact : mrtang@nudt.edu.cn   mrtang_cs@163.com
# @License : (C) All Rights Reserved


import proc_fun.proc_fun as procf
import socket
import threading
import struct
import time
import win32api,win32con
import math


class Py4C(threading.Thread):
    def __init__(self,filter = 3,userport = 5150,userip = '127.0.0.1',tobii4Cport=5151,tobii4Cip='127.0.0.1'):
        threading.Thread.__init__(self)

        self.useraddr = (userip,userport)
        self.tobii4caddr = (tobii4Cip,tobii4Cport)

        self.SCRW = win32api.GetSystemMetrics(win32con.SM_CXSCREEN)
        self.SCRH = win32api.GetSystemMetrics(win32con.SM_CYSCREEN)

        self.filter = filter
        self._xl = [0]*filter
        self._yl = [0]*filter

        self._lock = threading.Lock()

        self._gazexy = [-1,-1]
        self._q = False

        app = 'tobii_4c_app.exe'
        if procf.check_exsit(app):
            procf.kill_process(app)
            time.sleep(0.1)

        #启动服务，不显示黑框
        win32api.ShellExecute(0, 'open', 'tobii_4c_app.exe', '%i %s %i %s'%(userport,userip,tobii4Cport,tobii4Cip), '', 0)

        self.setDaemon(True)
        self.start()

    def quit(self):
        self._q = True
        for i in range(10): #等待子线程结束，未等到也强制退出
            if not self._q:
                break
            time.sleep(0.5)


    @property
    def gazeXY(self):
        return self._gazexy

    @property
    def gazepos(self):
        '''
        get gaze position on the screen
        :return: flg 0-gaze out of the screen, x,y is the original value
                 flg 1-gaze in the screen, x,y is the position of the screen
        '''
        x,y = self._gazexy
        if x<0 or x>1 or y<0 or y>1:
            return 0,x,y
        else:
            x = int(x * self.SCRW)
            y = int(y * self.SCRH)
            return 1,x,y

    def run(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(self.useraddr)

        while True:
            buf, addr = sock.recvfrom(128)
            x, y = struct.unpack('2f', buf)
            self._xl.append(x)
            self._xl.pop(0)
            self._yl.append(y)
            self._yl.pop(0)

            self._lock.acquire()
            self._gazexy = (sum(self._xl)/self.filter,sum(self._yl)/self.filter)
            self._lock.release()
            if self._q:
                sock.sendto(b'\x01\x03\x7d\x7f', self.tobii4caddr)
                break
        self._q = False


if __name__ == '__main__':
    import time
    p4c = Py4C()
    for i in range(5000):
        print(p4c.gazeXY)
        time.sleep(0.01)
    p4c.quit()




