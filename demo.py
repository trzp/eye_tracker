#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Time    : 2020/8/21 9:05
# @Version : 1.0
# @File    : demo.py
# @Author  : Jingsheng Tang
# @Version : 1.0
# @Contact : mrtang@nudt.edu.cn   mrtang_cs@163.com
# @License : (C) All Rights Reserved

import win32con
import win32api
import time
from py4c import *

p4c = Py4C(filter=3)
w,h = 1530,860

for i in range(5000):
    flg,x,y = p4c.gazepos
    if flg:
        win32api.SetCursorPos([x,y]) #设置鼠标位置
    time.sleep(0.01)

p4c.quit()
