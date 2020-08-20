# eye_tracker
使用stream_engine方式获取眼动数据（适用4C,5等消费级tobii眼动仪。需要使用tobii eye自带软件打开眼动仪才可读取数据流。tobii pro sdk方式无法读取，不支持该机型）

### 该程序使用vs2015开发，直接使用nuget管理器安装tobii_stream_engine_native包，即可开始创建程序

### 该程序的隐藏了控制台，目的是将其封装为一个后台应用，通过udp socket的方式向外转发眼动数据，提供更多其他程序设计语言使用

### 程序的调用： tobii_4c_app.exe 4000 127.0.0.1 4001 127.0.0.1, 参数：上位机（使用者）端口，ip，app端口，app的ip地址

### tobii_4c_app启动后，上位机可以通过socket接收眼动数据，可在某一次接收数据之后，向app发送4字节数据 1,3,125,127,该命令可使app关闭