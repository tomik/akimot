# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'main_window_with_log.ui'
#
# Created: Wed Feb 11 18:04:32 2009
#      by: PyQt4 UI code generator 4.4.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_window_main(object):
    def setupUi(self, window_main):
        window_main.setObjectName("window_main")
        window_main.resize(1173, 863)
        self.centralwidget = QtGui.QWidget(window_main)
        self.centralwidget.setObjectName("centralwidget")
        self.record = QtGui.QListWidget(self.centralwidget)
        self.record.setGeometry(QtCore.QRect(40, 690, 541, 121))
        self.record.setBatchSize(50)
        self.record.setSelectionRectVisible(False)
        self.record.setObjectName("record")
        self.button_back = QtGui.QPushButton(self.centralwidget)
        self.button_back.setGeometry(QtCore.QRect(590, 740, 106, 28))
        self.button_back.setObjectName("button_back")
        self.button_forward = QtGui.QPushButton(self.centralwidget)
        self.button_forward.setGeometry(QtCore.QRect(590, 780, 106, 28))
        self.button_forward.setObjectName("button_forward")
        self.board = QtGui.QLabel(self.centralwidget)
        self.board.setGeometry(QtCore.QRect(40, 20, 650, 650))
        self.board.setFrameShape(QtGui.QFrame.Box)
        self.board.setFrameShadow(QtGui.QFrame.Raised)
        self.board.setLineWidth(1)
        self.board.setObjectName("board")
        self.button_load = QtGui.QPushButton(self.centralwidget)
        self.button_load.setGeometry(QtCore.QRect(590, 700, 106, 28))
        self.button_load.setObjectName("button_load")
        self.log = QtGui.QTextBrowser(self.centralwidget)
        self.log.setGeometry(QtCore.QRect(710, 20, 441, 741))
        font = QtGui.QFont()
        font.setFamily("Monospace")
        self.log.setFont(font)
        self.log.setObjectName("log")
        self.button_search = QtGui.QPushButton(self.centralwidget)
        self.button_search.setGeometry(QtCore.QRect(730, 770, 106, 28))
        self.button_search.setObjectName("button_search")
        self.button_eval = QtGui.QPushButton(self.centralwidget)
        self.button_eval.setGeometry(QtCore.QRect(850, 770, 106, 28))
        self.button_eval.setObjectName("button_eval")
        self.slider_time_per_move = QtGui.QSlider(self.centralwidget)
        self.slider_time_per_move.setGeometry(QtCore.QRect(970, 770, 160, 25))
        self.slider_time_per_move.setMinimum(1)
        self.slider_time_per_move.setMaximum(10)
        self.slider_time_per_move.setSingleStep(1)
        self.slider_time_per_move.setSliderPosition(3)
        self.slider_time_per_move.setOrientation(QtCore.Qt.Horizontal)
        self.slider_time_per_move.setObjectName("slider_time_per_move")
        self.label_time_per_move = QtGui.QLabel(self.centralwidget)
        self.label_time_per_move.setGeometry(QtCore.QRect(1140, 770, 21, 20))
        self.label_time_per_move.setObjectName("label_time_per_move")
        window_main.setCentralWidget(self.centralwidget)
        self.menubar = QtGui.QMenuBar(window_main)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 1173, 27))
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
        self.button_search.setText(QtGui.QApplication.translate("window_main", "search", None, QtGui.QApplication.UnicodeUTF8))
        self.button_eval.setText(QtGui.QApplication.translate("window_main", "eval", None, QtGui.QApplication.UnicodeUTF8))
        self.label_time_per_move.setText(QtGui.QApplication.translate("window_main", "3", None, QtGui.QApplication.UnicodeUTF8))

