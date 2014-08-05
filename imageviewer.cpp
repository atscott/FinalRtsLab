 /****************************************************************************
 **
 ** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
 ** Contact: http://www.qt-project.org/legal
 **
 ** This file is part of the examples of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:BSD$
 ** You may use this file under the terms of the BSD license as follows:
 **
 ** "Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are
 ** met:
 **   * Redistributions of source code must retain the above copyright
 **     notice, this list of conditions and the following disclaimer.
 **   * Redistributions in binary form must reproduce the above copyright
 **     notice, this list of conditions and the following disclaimer in
 **     the documentation and/or other materials provided with the
 **     distribution.
 **   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
 **     of its contributors may be used to endorse or promote products derived
 **     from this software without specific prior written permission.
 **
 **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 ** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 ** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 ** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 ** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 ** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 ** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

 #include <QtGui>
 #include <unistd.h>
 #include "imageviewer.h"
 #include <sstream>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>

 pthread_t imageTakerThread;
 pthread_t imageFinderThread;
 int keepGoing;

 ImageViewer::ImageViewer()
 {
     imageLabel = new QLabel;
     imageLabel->setBackgroundRole(QPalette::Base);
     imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
     imageLabel->setScaledContents(true);

     scrollArea = new QScrollArea;
     scrollArea->setBackgroundRole(QPalette::Dark);
     scrollArea->setWidget(imageLabel);
     setCentralWidget(scrollArea);

     createActions();
     createMenus();

     setWindowTitle(tr("Picture Thang"));
     resize(500, 400);
 }

 void ImageViewer::start()
 {
     ImageViewer::deleteAlljpgs();
     keepGoing = 1;
     pthread_create(&imageTakerThread,NULL, takePictures ,(void *)this);

//     pthread_create(&imageFinderThread,NULL, scanForPictures ,(void *)this);
     ImageFinderWorker *workerThread = new ImageFinderWorker;
    // Connect our signal and slot
    connect(workerThread, SIGNAL(imageFound(QString)), SLOT(onImageFound(QString)));
    // Setup callback for cleanup when it finishes
    connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));
    // Run, Forest, run!
    workerThread->start(); // This invokes WorkerThread::run in a new thread

 }

 void ImageViewer::stop()
 {
     keepGoing = 0;
     ImageViewer::deleteAlljpgs();
 }

 void ImageViewer::createActions()
 {
     startAct = new QAction(tr("&Start"), this);
     startAct->setShortcut(tr("Ctrl+B"));
     connect(startAct, SIGNAL(triggered()), this, SLOT(start()));

     stopAct = new QAction(tr("&Stop"), this);
     stopAct->setShortcut(tr("Ctrl+Q"));
     connect(stopAct, SIGNAL(triggered()), this, SLOT(stop()));
 }

 void ImageViewer::createMenus()
 {
     fileMenu = new QMenu(tr("&File"), this);
     fileMenu->addAction(startAct);
     fileMenu->addSeparator();
     fileMenu->addAction(stopAct);

     menuBar()->addMenu(fileMenu);
 }

 void ImageViewer::adjustScrollBar(QScrollBar *scrollBar, double factor)
 {
     scrollBar->setValue(int(factor * scrollBar->value()
                             + ((factor - 1) * scrollBar->pageStep()/2)));
 }

 void* takePictures(void *args){
     if(args){}
    int counter=0;

    while(keepGoing == 1){
        std::string s;
        std::stringstream out;
        out << counter;
        s = out.str();
//        std::string command = std::string("gst-launch v4l2src device=/dev/video0 num-buffers=1 ! video/x-raw-yuv, framerate=5/1, width=320, height=240 ! ffmpegcolorspace ! queue ! jpegenc ! multifilesink location='frame") + s + ".jpg'\0";
        std::string command = std::string("gst-launch v4l2src device=/dev/video0 num-buffers=25 ! video/x-raw-yuv,framerate=5/1,'width=320,height=240'  ! ffmpegcolorspace ! jpegenc ! multifilesink location ='frame" + s + ".jpg'");
        const char * c = command.c_str();
        if(system(c)){}

        counter++;
    }

    return 0;
 }

 void ImageViewer::onImageFound(QString fileName)
 {
     if (!fileName.isEmpty()) {
         QImage image(fileName);
         if (image.isNull()) {
             qDebug() << "Could not load " << fileName;
             return;
         }
         imageLabel->setPixmap(QPixmap::fromImage(image));
         ImageViewer::fitToWindow();
         QFile::remove(fileName);
     }
 }

  void ImageViewer::fitToWindow()
  {
      scrollArea->setWidgetResizable(true);
  }

  void ImageViewer::deleteAlljpgs(){
      string dir = string(".");
      vector<string> files = vector<string>();

      getdir(dir,files);
      for (unsigned int i = 0;i < files.size();i++) {
          string::size_type loc = files[i].find( ".jpg", 0 );
          if( loc != string::npos ) {
              QString qstr = QString::fromUtf8(files[i].c_str());
              QFile::remove(qstr);
           }
      }
  }


 void ImageFinderWorker::run()
 {
         while(keepGoing == 1){
             string latest = getLatestPicture();
             cout << "found " <<latest << " to open." << endl;
             QString qstr = QString::fromUtf8(latest.c_str());
             emit imageFound(qstr);
             usleep(200000);
         }

 }

 int getdir (string dir, vector<string> &files)
 {
     DIR *dp;
     struct dirent *dirp;
     if((dp  = opendir(dir.c_str())) == NULL) {
         cout << "Error(" << errno << ") opening " << dir << endl;
         return errno;
     }

     while ((dirp = readdir(dp)) != NULL) {
         files.push_back(string(dirp->d_name));
     }
     closedir(dp);
     return 0;
 }

 string ImageFinderWorker::getLatestPicture()
 {
     string dir = string(".");
     vector<string> files = vector<string>();

     getdir(dir,files);

     int largestNumber = 0;
     string latestPicture;
     for (unsigned int i = 0;i < files.size();i++) {
         string::size_type loc = files[i].find( ".jpg", 0 );
         if( loc != string::npos ) {
             string fileNumberAsString = files[i].substr (5,loc-5);
             int fileNumber = atoi(fileNumberAsString.c_str());
             if(fileNumber > largestNumber){
                 largestNumber = fileNumber;
                 latestPicture = files[i];
             }

          }
     }
     return latestPicture;
 }
