from PyQt4 import QtGui
from PyQt4 import QtCore

import sys 
sys.path.append('..')

from aei import board 

ANIMATE_ONE = 0.2
DIRECTIONS = {'n' : -8, 'w' : -1, 'e' : 1, 's': 8} 

MAX_MOVE_INTERVAL = 1500
STEP_INTERVAL = 300

ERROR = -1
BSIDE = 8 
BSIZE = 64
COL_EMPTY = 2
COLORS = ['Gold', 'Silver', '']
PIECE_EMPTY_STR = 'Empty'
PIECES = {'r': 'Rabbit', 'c': 'Cat', 'd': 'Dog', 'h': 'Horse', 'm': 'Camel', 'e': 'Elephant', ' ' : PIECE_EMPTY_STR}
PIECES_NUM = ['r', 'c', 'd', 'h', 'm', 'e', ' ']
EDGE = 2.5

MODE_PLAY, MODE_REPLAY = range(2)

STEPSTR_LEN = 4

def stepstr2from(stepstr):
    return BSIZE - 1 - board.alg_to_index(stepstr[1:STEPSTR_LEN - 1])

def stepstr2to(stepstr):
    dir = stepstr[STEPSTR_LEN - 1]
    return stepstr2from(stepstr) + DIRECTIONS[dir]

def scale_pixmap(pixmap, size, ar=None):
    return pixmap.scaled(size.width(), size.height(), ar and ar or QtCore.Qt.KeepAspectRatio)

class BoardWidget(QtGui.QWidget):
    def __init__(self, widget):
        self.pos = None
        self.ps_stack = None
        self.mode = None

        self.mover = QtCore.QTimeLine(MAX_MOVE_INTERVAL)
        self.mover.setCurveShape(self.mover.LinearCurve)
        self.connect(self.mover, QtCore.SIGNAL("frameChanged(int)"), self.animate_move)

        widget = super(BoardWidget, self).__init__(widget) 

        self.square_width = 0
        self.square_height = 0
        self.repaint_index = None

        self.setMouseTracking(True)

        #actually marked squares
        self.marked = [] 
        self.last_upon = None

        return widget
        
    def compute_rec(self, index):
        rec  = self.geometry()
        left = rec.x() - EDGE + (index % BSIDE) * self.square_width
        top  = rec.y() - EDGE + (index / BSIDE) * self.square_height

        return QtCore.QRect(left, top, self.square_width, self.square_height) 
        
    def point2index(self, point):
        row = int(point.y() / self.square_height)
        col = int(point.x() / self.square_width)
        
        return BSIDE * row + col

    def resizeEvent(self, e):
        rec = self.geometry()
        self.board_pixmap = scale_pixmap(QtGui.QPixmap("img/BoardStoneBigCut.jpg"), rec)
        rec.setHeight(self.board_pixmap.height()) 
        rec.setWidth(self.board_pixmap.width()) 
        self.square_width  = (rec.width() - EDGE)/ float(BSIDE) 
        self.square_height = (rec.height() - EDGE)/ float(BSIDE) 

        self.repaint()

    def xmousePressEvent(self, e):
        index = self.point2index(e.pos())
        if not self.square_empty(index):
            return
        steps = filter(lambda x: stepstr2to(x) == index, self.marked)
        if steps == []:
            return 
        #there might be more steps with same prefix e.g. Eg5w, Eg5w hg4n, etc.
        step = steps[0][0:STEPSTR_LEN]
        steps = map(lambda x: x.partition(step)[2].strip(), steps)

        new_pos = self.pos.do_step(board.parse_move(step))
        print new_pos.to_short_str()
        new_steps = [ s for s in filter(lambda x: x != '', steps)]
        if '' in steps:
                new_steps += [ board.steps_to_str(step) for step, _ in new_pos.get_steps()]
        self.update_pos_inner(new_pos, new_steps)

    def xmouseMoveEvent(self, e):
        index = self.point2index(e.pos())
        #check whether cursor is upon new square
        if index != self.last_upon:
            self.last_upon = index
            if self.square_empty(index):
                if index not in map(stepstr2to, self.marked):
                    self.marked = []
                    self.repaint()
            #square with a piece -> update marked 
            else:
                state = self.ps_stack[-1]
                self.marked = [stepstr for stepstr in state[1] 
                               if stepstr2from(stepstr) == index and self.square_empty(stepstr2to(stepstr))]
                self.repaint()

    def paintEvent(self, e):
        painter = QtGui.QPainter(self)
        painter.drawPixmap(0, 0, self.board_pixmap)

        if self.pos:
            if self.repaint_index: 
                c = self.pos.to_short_str()[1:-1][self.repaint_index]
                player = (c == ' ' and COL_EMPTY or (c.islower() and 1 or 0))
                self.draw_square(painter, player, c.lower(), self.repaint_index)
                self.repaint_index = None
            else: #repaint all
                for index, c in enumerate(self.pos.to_short_str()[1:-1]):
                    player = (c == ' ' and COL_EMPTY or (c.islower() and 1 or 0))
                    self.draw_square(painter, player, c.lower(), index)

            if self.mode == MODE_PLAY:
                #draw marked squares
                for m in map(stepstr2to, self.marked):
                    self.draw_mark(painter, m)

    def start_move_animation(self, steps):
        self.steps = steps
        self.mover.setDuration(min(len(steps) * STEP_INTERVAL, MAX_MOVE_INTERVAL)) 
        self.mover.setCurrentTime(0)
        self.mover.setFrameRange(0, len(steps))
        self.mover.start()
            
    def animate_move(self, frame):
        if frame == 0:
            return
        (_, from_in), (_, to_in), step_piece, step_color = self.steps[frame - 1]
        to_in = BSIZE - to_in - 1
        from_in = BSIZE - from_in - 1
        self.pos = self.pos.do_move([self.steps[frame -1]])
        #erase previous
        if from_in != ERROR: #not an init => clear previous position
            index = from_in
            self.repaint_index = index
            self.repaint(self.compute_rec(index))

        if to_in == ERROR: #trapped
            index = to_in
        else: #ok
            index = to_in

        self.repaint_index = index
        self.repaint(self.compute_rec(index))

    def update_pos(self, pos):
        self.mover.stop()
        steps = [ board.steps_to_str(step) for step, _ in pos.get_steps()]
        self.update_pos_inner(pos, steps)

    def update_pos_inner(self, pos, steps):
        self.pos = pos
        self.ps_stack = [(pos, steps)]
        self.last_upon = None
        self.marked = []
        self.repaint()

    def square_empty(self, index):
        assert index >= 0 and index < BSIZE, "wrong square index"
        return self.pos.to_short_str()[1:-1][index] == ' '

    def draw_pixmap(self, painter, img, index):
        pixmap = scale_pixmap(QtGui.QPixmap(img), self.compute_rec(index))
        painter.drawPixmap(self.compute_rec(index).x(), self.compute_rec(index).y(), pixmap)

    def draw_square(self, painter, player, piece, index):
        s = "img/%s%s.gif" % (COLORS[player], PIECES[piece])
        self.draw_pixmap(painter, s, index)

    def draw_mark(self, painter, index):
        s = "img/mark.png" 
        self.draw_pixmap(painter, s, index)
