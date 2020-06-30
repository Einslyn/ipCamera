sudo cp cuda/include/cudnn.h /usr/local/cuda/include/
 
sudo cp cuda/lib64/libcudnn* /usr/local/cuda/lib64/
 
sudo chmod a+r /usr/local/cuda/include/cudnn.h
 
sudo chmod a+r /usr/local/cuda/lib64/libcudnn*


cudnn version

cat /usr/local/cuda/include/cudnn.h | grep CUDNN_MAJOR -A 2

# CUDA 10.0
conda install pytorch==1.2.0 torchvision==0.4.0 cudatoolkit=10.0 -c pytorch

conda install pytorch
https://blog.csdn.net/watermelon1123/article/details/88122020


pytorch old version
https://mirrors.tuna.tsinghua.edu.cn/anaconda/cloud/pytorch/linux-64/

anaconda install opencv
pip install opencv-python
