#!/usr/bin/python

import sys
from os.path import isfile
from PyQt4 import QtGui
from PyQt4 import QtCore

from window_main import Ui_window_main

sys.path.append('..')

from aei import board 

BSIZE = 64
BSIDE = 8 
ANIMATE_ONE = 0.2
MOVE_INTERVAL = 1500

ERROR = -1

LOG_SEP = '====='
LOG_WARNING = '[warning]' 

COL_EMPTY = 2
PIECE_EMPTY_STR = 'Empty'
COLORS = ['Gold', 'Silver', '']
PIECES = {'r': 'Rabbit', 'c': 'Cat', 'd': 'Dog', 'h': 'Horse', 'm': 'Camel', 'e': 'Elephant', ' ' : PIECE_EMPTY_STR}
PIECES_NUM = ['r', 'c', 'd', 'h', 'm', 'e', ' ']


class Tagui(QtGui.QMainWindow):

    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)
        self.ui = Ui_window_main()
        self.ui.setupUi(self)

        self.connect(self.ui.button_load, QtCore.SIGNAL('clicked()'), self.load_record)
        self.connect(self.ui.button_back, QtCore.SIGNAL('clicked()'), self.go_back)
        self.connect(self.ui.button_forward, QtCore.SIGNAL('clicked()'), self.go_forward)
        self.connect(self.ui.record, QtCore.SIGNAL('currentRowChanged(int)'), self.record_row_changed)
        self.mover = QtCore.QTimeLine(MOVE_INTERVAL)
        self.mover.setCurveShape(self.mover.LinearCurve)
        self.connect(self.mover, QtCore.SIGNAL("frameChanged(int)"), self.draw_pos_framed)

        self.create_board()
        self.log = self.ui.log
        self.log.active = False
        self.log.setReadOnly(True)
        self.ui.record.addItems(open('record_1').readlines())
        self.load_log('record_1')

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
        if isfile(fn):
            self.load_log(fn)

            self.ui.record.clear()
            self.clear_board()
            self.ui.record.addItems(open(fn).readlines())
            self.ui.record.setSelectionMode(self.ui.record.SingleSelection)

    def process_log_lines(self, lines):
        segments = []
        current = None
        for line in lines:
            if line.find(LOG_WARNING) != ERROR:
                continue
            if line.find(LOG_SEP) != ERROR:
                segments.append([])
                current = segments[-1]
            else:
                current.append(line)

        res = []
        for segment in segments:
            res.append(''.join(segment))
        
        return res
        

    def load_log(self, fn):
        fn = str(fn)
        dir = '/'.join(fn.split('/')[:-1])
        if dir != '':
            dir = "%s/" % dir
        name = fn.split('/')[-1]
        num = name.split('_')[-1]
        log_fn = "%slog_%s" % (dir, num)
        print fn
        print log_fn
        if isfile(log_fn):
            self.log.active = True
            self.log.clear()
            self.log.segments = self.process_log_lines(open(log_fn).readlines())
    
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
        pos, last = self.record_to_pos(items)
        self.animate_move(pos, last)
        self.log.setText(self.log.segments[(row)/2])
        #self.draw_pos(pos)
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
        if moves == []:
            return pos, []

        moves = map(lambda x: x[x.find(' ') + 1:], moves)
        for m in moves[:-1]: 
            try: 
                steps = board.parse_move(m)
                pos = pos.do_move(steps)
            except ValueError:
                break

        return pos, moves[-1]

    def draw_square(self, player, piece, index):
        s = "img/%s%s.gif" % (COLORS[player], PIECES[piece])
        self.squares[index].setPixmap(QtGui.QPixmap(s))

    def draw_empty(self, index):
        self.squares[index].setPixmap(self.empty_square_pic)
            

    def draw_pos_framed(self, frame):
        if frame == 0:
            return
        (_, from_in), (_, to_in), step_piece, step_color = self.steps[frame - 1]
        to_in = BSIZE - to_in - 1
        from_in = BSIZE - from_in - 1
        self.pos = self.pos.do_step([self.steps[frame -1]])
        #erase previous
        if from_in != ERROR: #not an init
            self.draw_empty(from_in)
        #put new
        if to_in == ERROR: 
            self.draw_empty(from_in)
        else:
            self.draw_square(step_color, PIECES_NUM[step_piece], to_in)   

    def animate_move(self, pos, last_move):
        if not last_move: 
            self.draw_pos(pos)
            return 
        steps = board.parse_move(last_move)
        #steps = [step for step in steps if step[1][0] != 0] 

        self.pos = pos
        self.draw_pos(pos)
        self.steps = steps
        self.mover.stop()
        self.mover.setCurrentTime(0)
        self.mover.setFrameRange(0, len(steps))
        self.mover.start()
        
    def draw_pos(self, pos):
        for index, c in enumerate(pos.to_short_str()[1:-1]):
            player = (c == ' ' and COL_EMPTY or (c.islower() and board.COL_SILVER or board.COL_GOLD))
            self.draw_square(player, c.lower(), index)
        pass


app = QtGui.QApplication(sys.argv)
icon = Tagui()
icon.show()
app.exec_()

