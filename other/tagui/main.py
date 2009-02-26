#!/usr/bin/python

import sys
from os.path import isfile
from PyQt4 import QtGui
from PyQt4 import QtCore

from window_main import Ui_window_main

sys.path.append('..')

from aei.aei import StdioEngine, EngineController 
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
        self.log = self.ui.log
        self.record = self.ui.record
        self.record.last_row = self.record.currentRow()

        self.connect(self.ui.button_load, QtCore.SIGNAL('clicked()'), self.load_record)
        self.connect(self.ui.button_back, QtCore.SIGNAL('clicked()'), self.go_back)
        self.connect(self.ui.button_forward, QtCore.SIGNAL('clicked()'), self.go_forward)
        self.connect(self.record, QtCore.SIGNAL('currentRowChanged(int)'), self.record_row_changed)
        self.mover = QtCore.QTimeLine(MOVE_INTERVAL)
        self.mover.setCurveShape(self.mover.LinearCurve)
        self.connect(self.mover, QtCore.SIGNAL("frameChanged(int)"), self.draw_pos_framed)
        self.connect(self.ui.button_search, QtCore.SIGNAL("clicked()"), self.start_search)
        self.connect(self.ui.slider_time_per_move, QtCore.SIGNAL("valueChanged(int)"), self.time_per_move_changed)

        self.create_board()
        self.log.active = False
        self.log.setReadOnly(True)

        #just debug
        #self.record.addItem('record')
        #self.record.addItems(open('record_1').readlines())
        #self.load_log('record_1')
        #self.record.setCurrentRow(0)

        engine_cmd = './akimot -c default.cfg -l'
        self.engine = EngineController(StdioEngine(engine_cmd, None))
        self.time_per_move_changed(self.ui.slider_time_per_move.value())
        self.pos, _ = self.get_pos(0)

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

            self.record.clear()
            self.record.addItem('record')
            self.clear_board()
            self.record.addItems(open(fn).readlines())
            self.record.setSelectionMode(self.record.SingleSelection)
            self.record.setCurrentRow(0)

    def process_log_lines(self, lines):
        segments = []
        current = []
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
        if isfile(log_fn):
            self.log.active = True
            self.log.clear()
            self.log.segments = self.process_log_lines(open(log_fn).readlines())

            #log might come frmo both players ( 2 akimots playing each other ) ... dirty
            if 2 * len(self.log.segments) < self.record.count():
                self.log.index_func = lambda x: x/2
            else: 
                self.log.index_func = lambda x: x
    
    def go_back(self):
        row = self.record.currentRow() 
        assert row > 0, "cannot go back"
        self.record.setCurrentRow(row - 1)
        
    def go_forward(self):
        row = self.record.currentRow() 
        assert row < self.record.count() - 1, "cannot go forward"
        self.record.blockSignals(True) 
        row += 1
        self.record.setCurrentRow(row)
        self.record.blockSignals(False) 
        self.animate_move(row)
        self.record_update()

    def record_row_changed(self):
        row = self.record.currentRow()
        if row - self.record.last_row == 1: 
            self.animate_move(row)
            self.record_update()
            return
        self.pos, last = self.get_pos(row)
        self.draw_pos(self.pos)
        self.record_update()

    def record_update(self):
        row = self.record.currentRow()
        self.record.last_row = row
        #if row > 0:
        try:
            self.log.setText(self.log.segments[self.log.index_func(row)])
        except IndexError: 
            pass
        self.ui.button_back.setEnabled(True)
        self.ui.button_forward.setEnabled(True)
        if row == 0: 
            self.ui.button_back.setEnabled(False)
        if row >= self.record.count() - 1:
            self.ui.button_forward.setEnabled(False)

    def make_empty_pos(self):
        bitboards = list([list(x) for x in board.BLANK_BOARD])
        return board.Position(board.COL_GOLD, 4, bitboards)

    def get_pos(self, row, separate_last_move=False):
        moves = map(lambda x: str(x.text()).strip(), [self.record.item(index) for index in xrange(1, row + 1)])
        pos = self.make_empty_pos()
        if row == 0: #moves == ['record'] or moves == []:
            return pos, []

        if not separate_last_move:
            moves.append(' ')

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
        if to_in == ERROR: #trapped
            self.draw_empty(from_in)
        else: #ok
            self.draw_square(step_color, PIECES_NUM[step_piece], to_in)   

    def animate_move(self, move_num):

        self.pos, last_move = self.get_pos(move_num, separate_last_move=True)
        if not last_move: 
            self.draw_pos(self.pos)
            return 
        steps = board.parse_move(last_move)

        self.draw_pos(self.pos)
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

    def time_per_move_changed(self, ttm):
        self.engine.ttm = ttm
        self.ui.label_time_per_move.setText(str(ttm))

    def start_search(self):
        self.engine.newgame()
        self.engine.setoption('tcmove', '%s' % self.engine.ttm )
        self.engine.setposition(self.pos)
        self.engine.go() 

        self.log.clear()
        self.log.setText('Search results ...\n')

        while True:
            resp = self.engine.get_response()
            if resp.type == "bestmove":
                resp.message = resp.move
            self.log.append(resp.message)
            if resp.type == "log":
                if resp.message.split(' ')[-1] == "over":
                    break

    def __del__(self): 
        self.engine.quit()
        self.engine.cleanup()


app = QtGui.QApplication(sys.argv)
icon = Tagui()
icon.show()
app.exec_()


