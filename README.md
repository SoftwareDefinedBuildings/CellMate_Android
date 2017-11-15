# SnapLink : Visual Appliance Identification and Control in Smart Buildings
[![Build Status](https://travis-ci.com/kaifeichen/SnapLink_Android.svg?token=XjizLR77Z2rgJhyHZZ73&branch=master)](https://travis-ci.com/kaifeichen/SnapLink_Android)

## What is SnapLink?
Please refer to the README.md in https://github.com/SoftwareDefinedBuildings/SnapLink

## How to Install SnapLink Android Client?

1. Enable installing applications from unknown sources from the Android phone settings. More details can be found [here](https://developer.android.com/distribute/tools/open-distribution.html).

2. Download the latest apk file from [the SnapLink Android releases](https://github.com/SoftwareDefinedBuildings/SnapLink_Android/releases).

2. Open the apk file to install.

## How to Work on SnapLink Android Client?

1. Download [Android Studio](https://developer.android.com/studio/index.html) and install [Android NDK](https://developer.android.com/ndk/index.html).

2. Clone this repository

  ```
  cd ~/workspace
  git clone https://github.com/SoftwareDefinedBuildings/SnapLink_Android
  ```

3. Import this project in Android Studio.

## Privacy policy

This app is designed to collect information for a research study at the University of California, Berkeley. The study is being led by Kaifei Chen <kaifei@berkeley.edu>.

The goal of this study is to show how a robust smart phone based image localization system can be used to augment human building interactions.
This app allows users to point their smartphone camera to an appliance to identify and interact with it.
To achieve this, we collect smartphone camera images and other sensor data to provide a fast and accurate estimation of the 3D location and orientation of the phone.
The collected data will be saved and used to improve the system.
At the moment of writing, all communications to our server are over GRPC and not encrypted. We plan to add authentication and encryption later.


## Citation
You must cite the following paper if you use this code in any ways:

```
@inproceedings{chen2015intuitive,
  title={Intuitive Appliance Identification using Image Matching in Smart Buildings},
  author={Chen, Kaifei and Kolb, John and F{\"u}rst, Jonathan and Hong, Dezhi and Katz, Randy H},
  booktitle={Proceedings of the 2nd ACM International Conference on Embedded Systems for Energy-Efficient Built Environments},
  pages={103--104},
  year={2015},
  organization={ACM}
}
```

## License

```
Copyright (c) 2016, Regents of the University of California
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

 - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the
   distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
```

## Questions?

Please Email Kaifei Chen <kaifei@berkeley.edu> or Tong Li <sasrwas@berkeley.edu>
