## 编译

~~~
# gcc t.c -I /home/lcc/lccRoot/programs/ffmpeg-3.1.11/include/ \
        -L /home/lcc/lccRoot/programs/ffmpeg-3.1.11/lib/ \
        -lavutil -lavformat -lavcodec -lavutil -lm -g -lswscale
~~~

## 执行

~~~
# ./a.out test.avi
~~~
