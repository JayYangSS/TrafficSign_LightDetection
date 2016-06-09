# TrafficSignDetection
implement the traffic sign detection

This project can user the shape information to detect the traffic signs. Using the IHLS color space to get rid of 
the disturbance of the color of the complicated background.

After getting the ROI through the color information, I extract the contour of the ROI and get the shape information,
combine shape+color information, I classify the traffic signs into several categories, and in each category, I use the 
BP neural network to recognize the traffic signs accurately.

On the master branch, this project doesn't use Kalman Filter, so the directory`KalmanFilter`,`HungarianAlg`,`Tracker` are excluded from this project. The structure of the project is like this:

![TLR](http://7xniym.com1.z0.glb.clouddn.com/TLR_Structure.png)