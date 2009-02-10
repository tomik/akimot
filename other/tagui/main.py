#!/usr/bin/python

import sys
from PyQt4 import QtGui
from PyQt4 import QtCore

from window_main import Ui_window_main

sys.path.append('..')

from aei import board 

BSIZE = 64
BSIDE = 8 

COL_EMPTY = 2
COLORS = ['Gold', 'Silver', '']
PIECES = {'r': 'Rabbit', 'c': 'Cat', 'd': 'Dog', 'h': 'Horse', 'm': 'Camel', 'e': 'Elephant', ' ' : 'Empty'}

class Tagui(QtGui.QMainWindow):

    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)
        self.ui = Ui_window_main()
        self.ui.setupUi(self)

        self.connect(self.ui.button_load, QtCore.SIGNAL('clicked()'), self.load_record)
        self.connect(self.ui.button_back, QtCore.SIGNAL('clicked()'), self.go_back)
        self.connect(self.ui.button_forward, QtCore.SIGNAL('clicked()'), self.go_forward)
        self.connect(self.ui.record, QtCore.SIGNAL('currentRowChanged(int)'), self.record_row_changed)

        self.create_board()

         
        self.ui.record.addItems(open('record').readlines())

    def create_board(self):

        self.board = self.ui.board
        pic_board = QtGui.QPixmap('img/BoardStoneBig.jpg')
        self.board.setPixmap(pic_board)

        square_width = pic_board.width() / float(BSIDE) 
        square_height = pic_board.height() / float(BSIDE) 

        base_left = self.board.x()
        base_top = self.board.y() 
        self.squares = [] 
        self.empty_square_pic = QtGui.QPixmap('img/Empty.gif')
        for i in xrange(BSIZE):
            self.squares.append(QtGui.QLabel(self))
            square = self.squares[i]
            left = base_left + (i % BSIDE ) * square_width
            top = base_top + (i / BSIDE)* square_height
            square_pic = self.empty_square_pic
            square.setPixmap(square_pic)
            square.setGeometry(left, top, square_pic.width(), square_pic.height())

    def clear_board(self):

        for s in self.squares:
            s.setPixmap(self.empty_square_pic)

    def load_record(self):
        fd = QtGui.QFileDialog(self)
        fn = fd.getOpenFileName()
        from os.path import isfile
        if isfile(fn):
            self.ui.record.clear()
            self.clear_board()
            self.ui.record.addItems(open(fn).readlines())
            self.ui.record.setSelectionMode(self.ui.record.SingleSelection)

    
    def go_back(self):
        current = self.ui.record.currentRow() 
        assert current > 0, "cannot go back"
        self.ui.record.setCurrentRow(current - 1)
        

    def go_forward(self):
        current = self.ui.record.currentRow() 
        assert current < self.ui.record.count() - 1, "cannot go forward"
        self.ui.record.setCurrentRow(current + 1)

    def record_row_changed(self, row):
        items = map(lambda x: str(x.text()).strip(), [self.ui.record.item(index) for index in xrange(0, row + 1)])
        self.draw_pos(self.record_to_pos(items))
        self.ui.button_back.setEnabled(True)
        self.ui.button_forward.setEnabled(True)
        if row == 0: 
            self.ui.button_back.setEnabled(False)
        if row >= self.ui.record.count() - 1:
            self.ui.button_forward.setEnabled(False)

    def make_empty_pos(self):
        bitboards = list([list(x) for x in board.BLANK_BOARD])
        return board.Position(board.COL_GOLD, 4, bitboards)

    def record_to_pos(self, moves):
        pos = self.make_empty_pos()

        for m in moves: 
            m = m[m.find(' ') + 1:]
            try: 
                steps = board.parse_move(m)
                pos = pos.do_move(steps)
            except ValueError:
                break

        return pos

    def draw_square(self, player, piece, index):
        s = "img/%s%s.gif" % (player, piece)
        self.squares[index].setPixmap(QtGui.QPixmap(s))


    def draw_pos(self, pos):
        for index, c in enumerate(pos.to_short_str()[1:-1]):
            player = (c == ' ' and COL_EMPTY or (c.islower() and board.COL_SILVER or board.COL_GOLD))
            self.draw_square(COLORS[player], PIECES[c.lower()], index)
        pass
                


app = QtGui.QApplication(sys.argv)
icon = Tagui()
icon.show()
app.exec_()

