# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'main_window.ui'
#
# Created: Tue Feb 10 23:35:22 2009
#      by: PyQt4 UI code generator 4.4.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_window_main(object):
    def setupUi(self, window_main):
        window_main.setObjectName("window_main")
        window_main.resize(997, 726)
        self.centralwidget = QtGui.QWidget(window_main)
        self.centralwidget.setObjectName("centralwidget")
        self.record = QtGui.QListWidget(self.centralwidget)
        self.record.setGeometry(QtCore.QRect(720, 70, 231, 551))
        self.record.setObjectName("record")
        self.button_back = QtGui.QPushButton(self.centralwidget)
        self.button_back.setGeometry(QtCore.QRect(720, 640, 106, 28))
        self.button_back.setObjectName("button_back")
        self.button_forward = QtGui.QPushButton(self.centralwidget)
        self.button_forward.setGeometry(QtCore.QRect(830, 640, 106, 28))
        self.button_forward.setObjectName("button_forward")
        self.board = QtGui.QLabel(self.centralwidget)
        self.board.setGeometry(QtCore.QRect(40, 20, 650, 650))
        self.board.setFrameShape(QtGui.QFrame.Box)
        self.board.setFrameShadow(QtGui.QFrame.Raised)
        self.board.setLineWidth(1)
        self.board.setObjectName("board")
        self.button_load = QtGui.QPushButton(self.centralwidget)
        self.button_load.setGeometry(QtCore.QRect(720, 30, 106, 28))
        self.button_load.setObjectName("button_load")
        window_main.setCentralWidget(self.centralwidget)
        self.menubar = QtGui.QMenuBar(window_main)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 997, 27))
        self.menubar.setObjectName("menubar")
        window_main.setMenuBar(self.menubar)
        self.statusbar = QtGui.QStatusBar(window_main)
        self.statusbar.setObjectName("statusbar")
        window_main.setStatusBar(self.statusbar)

        self.retranslateUi(window_main)
        QtCore.QMetaObject.connectSlotsByName(window_main)

    def retranslateUi(self, window_main):
        window_main.setWindowTitle(QtGui.QApplication.translate("window_main", "tagui", None, QtGui.QApplication.UnicodeUTF8))
        self.button_back.setText(QtGui.QApplication.translate("window_main", "back", None, QtGui.QApplication.UnicodeUTF8))
        self.button_forward.setText(QtGui.QApplication.translate("window_main", "forward", None, QtGui.QApplication.UnicodeUTF8))
        self.board.setText(QtGui.QApplication.translate("window_main", "TextLabel", None, QtGui.QApplication.UnicodeUTF8))
        self.button_load.setText(QtGui.QApplication.translate("window_main", "load", None, QtGui.QApplication.UnicodeUTF8))

