
# These routines were originally developed for the perl
#   move count program which is availible under the
#   download section of the arimaa.com site.  
#   They are also used by the gameserver to check for valid moves.
#   So if any bugs are found, that code should be fixed also.
# Please notify Omar if you find any bugs. Thanks.
# 
# These routines are used by the match script to check if the move
#   made by a bot is valid or not.

sub PositionUpdate{
  local($fn, $m) = @_;
  local($c, $t, $s, @p, @steps, $bs, $ps, $rep);
  local($pieceType,$pieceCol,$from,$to,$i,%ch);
  local($mm, $st, $push, $lastStepIsPush);
  local(@np, $np);

  $m =~ s/ +/ /g;
  $m =~ s/^ +//;
  $m =~ s/ +$//;

  if ($m eq ""){
    return (0, "no steps taken m=[$m]");
  }

  ($c,$t,$s,@p) = Pos::readBoard($fn);
  if ($c eq ""){ 
    $c = 1; 
    $t = "w";
    $s = "";
    @p = split(//,"                                                                ");
  }

#goto skipPosCheck;

# check setup of pieces
  if ($c == 1){
    @steps = split(/ /, $m);
    if ($#steps+1 != 16){
      return (0, "invalid number of steps in setup ($#steps) $m");
    }
    foreach $s (@steps){
      ($pieceType, $pieceCol, $from, $to) = Pos::stepInfo($s);
      if ($from != -1){
        return (0, "invalid step $s");
      }
      $ch{$pieceType} += 1;
      $ch{"$to"} += 1;
    }
    if ($t eq "w"){
      for($i=48;$i<64;$i++){
        if ($ch{$i} != 1){
          return (0, "invalid setup of first 2 rows $m");
        }
      }
      if (! ($ch{R}==8 && $ch{C}==2 && $ch{D}==2 && $ch{H}==2 && $ch{M}==1 && $ch{E}==1)){
        return (0, "invalid number of piece type");
      }
    }
    else{
      for($i=0;$i<16;$i++){
        if ($ch{$i} != 1){
          return (0, "invalid setup of first 2 rows $m");
        }
      }
      if (! ($ch{r}==8 && $ch{c}==2 && $ch{d}==2 && $ch{h}==2 && $ch{m}==1 && $ch{e}==1)){
        return (0, "invalid number of piece type");
      }
    }

# create the position file after taking the steps
    @np = Pos::takeSteps($m, @p);
    $c = ($t eq "b")?$c+1:$c;
    $t = ($t eq "b")?"w":"b";
    Pos::writeBoard($fn, $c, $t, "", @np);
# 2005.05.29 Omar
#   changed the following line; we are now adding side to move to
#   the position history file. Search the Arimaa forums for 'abcda'
#   or "side to move" in year 2004 for details.
#    PositionAddHist($fn, join('', @np));
    PositionAddHist($fn, join('', @np) . $t);
    return (1);
  }

# check regular move
# we check the move by generating all the valid steps from the current position
#   and checking to see if one of them matches the current step in the given move
#   if so we remove that step from the given move, change the board position
#   and check the next step until all steps have been removed from the given move
  $mm = $m;
  $st = "";
  while ($mm ne ""){
    @np = Pos::takeSteps($st, @p);
    @steps = Pos::getSteps($t, $st, @np);
    $bs = "";
#`echo "*** $mm *** $st" >> /tmp/steps`;
    `echo "steps:" >> /tmp/steps`;
    foreach $ps (@steps){
      `echo "  $ps" >> /tmp/steps`;
      ($ps, $push) = ($ps =~ m/([^\-]*)(\-)?$/);
      if ($mm =~ m/^$ps/){
        if (length($ps) > length($bs)){
          $bs = $ps;
          $lastStepIsPush = $push;
        }
      }
    }
    if ($bs eq ""){
#`echo "could not find valid step in $mm" >> /tmp/pos`;
      return (0, "bad step $mm");
    }
    $mm =~ s/^$bs//;
    $mm =~ s/^ +//;
    $st .= " $bs";
    $st =~ s/^ +//;
  }
  if ($lastStepIsPush ne ""){
    return (0, "push not completed");
  }

# make sure move changes the state of the board
  @np = Pos::takeSteps($m, @p);
  $np = join('', @np);
  if ($np eq join('',@p)){
    return (0, "position not changed");
  }

# 2008.07.01
# make sure this move does not repeat the position 3 times, such moves are
#   considered invalid and are ignored rather than causing a lose.
`echo "checking repeats" >> /tmp/repc`;
  $rep = PositionCheckRepeat($fn, $np);
  if ($rep ne ''){
    return(0, "position repeats for the 3rd time");
  }

skipPosCheck:

# create the position file after taking the steps
#  @np = Pos::takeSteps($m, @p);
  $c = ($t eq "b")?$c+1:$c;
  $t = ($t eq "b")?"w":"b";
  Pos::writeBoard($fn, $c, $t, "", @np);
# 2005.05.29 Omar
#   changed the following line:
#  PositionAddHist($fn, join('', @np));
  PositionAddHist($fn, join('', @np) . $t);

  return (1);
}

sub PositionCheckGoal{
  local($fn) = @_;
  my($i, $c, $t, $s, @p);

# Omar Syed 2004.02.11
#   It is possible that both players rabbits have reached goal.
#   In such cases the player who is making the move wins.
#   So the order of the checking matters
#   Note that $t is the player who's turn it is now; so the
#   other player is the one who moved last; if $t='w' then 'b' moved last

  ($c,$t,$s,@p) = Pos::readBoard($fn);

  if ($t eq 'b'){ # if it is now silvers turn check if gold won first
    for($i=0;$i<8;$i++){
      if ($p[$i] eq 'R'){ return 'w'; }
    }
    for($i=56;$i<64;$i++){
      if ($p[$i] eq 'r'){ return 'b'; }
    }
  }
  else{ # if it is now golds turn check if silver won first
    for($i=56;$i<64;$i++){
      if ($p[$i] eq 'r'){ return 'b'; }
    }
    for($i=0;$i<8;$i++){
      if ($p[$i] eq 'R'){ return 'w'; }
    }
  }
  return '';
}

# 2008.07.01 Added this to check for win by lose of all rabbits. See http://arimaa.com/arimaa/forum/cgi/YaBB.cgi?board=talk;action=display;num=1206399208
#   first check if opponent has lost all rabbits then if we lost all rabbits.
#   So the order of the checking matters
#   Note that $t is the player who's turn it is now; so the
#   other player is the one who moved last; if $t='w' then 'b' moved last
sub PositionCheckElimination{
  local($fn) = @_;
  my($i, $c, $t, $s, @p, $w);

  ($c,$t,$s,@p) = Pos::readBoard($fn);
  if ($c < 2){ return ''; } # don't check on setup moves
  if ($t eq 'b'){ # if it is now silvers turn check if gold won first
    $w = 'w';
    for($i=0;$i<64;$i++){
      if ($p[$i] eq 'r'){ $w = ''; }
    }
    if ($w ne ''){ return $w; }
    $w = 'b';
    for($i=0;$i<64;$i++){
      if ($p[$i] eq 'R'){ $w = ''; }
    }
    if ($w ne ''){ return $w; }
  }
  else{ # if it is now golds turn check if silver won first
    $w = 'b';
    for($i=0;$i<64;$i++){
      if ($p[$i] eq 'R'){ $w = ''; }
    }
    if ($w ne ''){ return $w; }
    $w = 'w';
    for($i=0;$i<64;$i++){
      if ($p[$i] eq 'r'){ $w = ''; }
    }
    if ($w ne ''){ return $w; }
  }
  return '';
}

# 2008.07.01 Since 3rd time repetition is no longer a losing condition,
#   but rather just not allowed, it is possible that a player has only
#   one move an it is causes a 3rd time repetition. In this case the player
#   loses by immobilization. It may also be possible that a player has
#   more than one move and all of those case a repeat; we don't check for this.
#   The players moves will not be accepted and they would lose on time.
sub PositionCheckOnlyMoveIsRepeat{
  my($fn) = @_;
  my($c, $t, $s, @p, @steps, $op);
  my($oneMove, $ps, $push, $rep);
  local(*FH);

  ($c,$t,$s,@p) = Pos::readBoard($fn);
  $op = ($t eq 'w')?'b':'w';

# don't check setup moves
  if ($c < 2){ return ''; }

  $oneMove = 1;
  @steps = Pos::getSteps($t, $s, @p);
# $#steps = -1 if no moves, =0 if one move, =1 if two moves, etc
  while($#steps >= 0){
    if ($#steps > 0){
      $oneMove = 0; last;
    }
    $st = $steps[0];
    ($st, $push) = ($st =~ m/([^\-]*)(\-)?$/);
    $s .= " $st";
    $s =~ s/^ +//;
    @np = Pos::takeSteps($s, @p);
    @steps = Pos::getSteps($t, $s, @np);
  }
  if ($s eq ''){ return ''; }
  if ($oneMove == 0){ return ''; }
  $st = join('', @np) . $t;
  open(FH, "<${fn}.hist");
  @p = <FH>;
  close FH;
  foreach $ps (@p){
    if ($ps eq $st){ $rep += 1; }
  }
  if ($rep >= 3){
     return $t;
  }
  return '';
}


# 2008.07.01 This is no longer used. See http://arimaa.com/arimaa/forum/cgi/YaBB.cgi?board=talk;action=display;num=1206399208
#   draws are no longer allowed.
sub PositionCheckDraw{
  local($fn) = @_;
  my($i, $c, $t, $s, @p);

  ($c,$t,$s,@p) = Pos::readBoard($fn);
  if ($c < 2){ return ''; }
  for($i=0;$i<64;$i++){
    if ($p[$i] eq 'R'){ return ''; }
    if ($p[$i] eq 'r'){ return ''; }
  }
  return 'd';
}

# Omar changed this on 2003.05.27 so that only the
#   moves for the player whos turn it is is checked
#   and if that player cannot make a move then the game ends
sub PositionCheckStalemate{
  local($fn) = @_;
  my($i, $c, $t, $s, @p, @steps);

  ($c,$t,$s,@p) = Pos::readBoard($fn);
  if ($c < 2){ return ''; }
  if ($t eq 'w'){
    @steps = Pos::getSteps('w', $s, @p);
    if ($#steps<0){ return 'b'; }
  }
  else{
    @steps = Pos::getSteps('b', $s, @p);
    if ($#steps<0){ return 'w'; }
  }
  return '';
}

sub PositionCheckRepeat{
  local($fn, $np) = @_;
  local($p, @p, $last, $rep, $c, $t, $nt, $s);
  local(*FH);

  ($c,$t,$s,@p) = Pos::readBoard($fn);
  $nt = ($t eq 'w')?'b':'w';
#  @p = `/bin/cat ${fn}.hist`;
  open(FH, "<${fn}.hist");
  @p = <FH>;
  close FH;
#  $last = $p[$#p];
  $last = $np.$nt;
  foreach $p (@p){
    chomp $p;
    if ($p eq $last){ $rep += 1; }
  }
  if ($rep >= 2){
     return $t;
  }
  return '';
}

sub PositionWinByScore{
  my($fn) = @_;
  my(@p, $p, $sc);
  local(*FH);

  open(FH, "<${fn}.hist");
  @p = <FH>;
  close FH;
  shift(@p);
  @p = reverse(@p);
  for $p (@p){
    $sc = PositionScorePosition($p);
    if ($sc != 0){ last; }
  }
  if ($sc>0){ return 'w'; }
  return 'b';
}

sub PositionScorePosition{
  my($b) = @_;
  my(@b, $p, $g, $s);

  @b = split('', $b);
  foreach $p (@b){
    if ($p eq 'R'){ $g += 1; }
    if ($p eq 'C'){ $g += 1; }
    if ($p eq 'D'){ $g += 1; }
    if ($p eq 'H'){ $g += 1; }
    if ($p eq 'M'){ $g += 1; }
    if ($p eq 'E'){ $g += 1; }
    if ($p eq 'r'){ $s += 1; }
    if ($p eq 'c'){ $s += 1; }
    if ($p eq 'd'){ $s += 1; }
    if ($p eq 'h'){ $s += 1; }
    if ($p eq 'm'){ $s += 1; }
    if ($p eq 'e'){ $s += 1; }
  }
  return $g-$s;
}

sub PositionAddHist{
  local($fn, $p) = @_;
  local(*FH);

  open(FH, ">>${fn}.hist");
  print FH "$p\n";
  close FH;
}

sub PositionDeleteHist{
  local($fn) = @_;
  local(*FH, $p, @p);

#  @p = `/bin/cat ${fn}.hist`;
  open(FH, "<${fn}.hist");
  @p = <FH>;
  close FH;
  delete $p[$#p];
  $p = join('', @p);
  open(FH, ">${fn}.hist");
  print FH "$p";
  close FH;
}

# Uses the latest entry in the game position history file
#   to create the position file; used during takebacks
sub PositionUpdateFromHist{
  local($fn) = @_;
  local($c, $t, $s, @p, $pos, @pos, @np);
  local(*FH);

  ($c,$t,$s,@p) = Pos::readBoard($fn);
#  @pos = `/bin/cat ${fn}.hist`;
  open(FH, "<${fn}.hist");
  @pos = <FH>;
  close FH;
  $pos = $pos[$#pos];
  chomp($pos);
# 2005.05.29 Omar 
#   added the chop() line to remove the 'w' or 'b' at the end of the line
#   we are now adding side to move in the position history file
  chop($pos);
  if ($pos eq ""){
    $pos = "                                                                ";
  }
  @np = split('', $pos);
  $t = $#pos+1;
  $c = int($t/2)+1;
  $t = ($t%2 == 0)?'w':'b';
  Pos::writeBoard($fn, $c, $t, "", @np);
}

sub PositionMakeNewBoard{
  local($fn) = @_;
  local(@p, $pos);

  $pos = "                                                                ";
  @p = split('', $pos);
  Pos::writeBoard($fn, 1, 'w', "", @p);
}


package Pos;


sub saveInitialPos{
  local($t,$s,@p) = @_;

# take back any steps that have already been taken and save
#   that board position, but don't count it.
  if ($s){
    addPosition($s, @p);
  }
  @p = takeBackSteps($s, @p);
  addPosition(" ", @p);
  $PosMul -= 1;
  $PosCount -= 1;
}

sub genMoves{
  local($t, $s, @p) = @_;
  local(@steps, $step, @np, $ns, $push);

  @steps = getSteps($t, $s, @p);
  foreach $step (@steps){
    ($step, $push) = ($step =~ m/([^\-]*)(\-)?$/);
    @np = takeSteps($step, @p);
    $ns = "$s $step";
    $ns =~ s/^ +//;
    if ($push ne "-"){
      addPosition($ns, @np);
    }
    genMoves($t, $ns, @np);
  }
}

sub addPosition{
  local($s, @p) = @_;
  local($p, $os, $fr);

  $PosMul += 1;
  $p = join('', @p);
  $os = $Pos{$p};
  if (($os eq "") || (length($s) < length($os))){
    $Pos{$p} = $s;
    if ($os eq ""){ $PosCount += 1; }
  }
  if ($PosMul%1000 == 0){ 
    $fr = int(10000*$PosCount/$PosMul)/100;
    if ($interactive){
      if ($PosMul==1000){
        print "Unique Toal %\n";
      }
      print "$PosCount $PosMul  $fr\n"; 
    }
  }
}

sub showMoves{
  local($k, $v, $fr);

  while(($k, $v)=each(%Pos)){
    if ($v =~ m/\w/){
      print "$v\n";
    }
  }
}



# Note that steps which start a push are marked with a
#   dash (-) at the end; to indicate that they need to 
#   be completed.
sub getSteps{
  local($t,$s,@p) = @_;
  local(@s,$ptype,$pcol,$fi,$ni,@a,$a,$type,$col,@np,$ti,@steps);
  local($i,$xd,$pulled,$ls,$stepsLeft,$lastStep,$pstr);
  local($pulledto);

#print "** in getSteps\n";

  @s = split(/ /, $s);
  $stepsLeft = 4;
  $lastStep = "";
  foreach(@s){ 
    if ($_ !~ m/x$/){ 
      $lastStep = $_;
      $stepsLeft -= 1; 
    }
  }
#print "steps Left is $stepsLeft; last step is $lastStep\n";
  if ($stepsLeft <= 0){ return; }
  ($ptype,$pcol, $fi, $ni) = stepInfo($lastStep);
  $ls = lastStepType($t, @s); # 0=?? 1=push -1=pull
#print "last steps Type is $ls\n";
  if (($ptype ne " ") && ($pcol ne $t) && (1) && ($ls != -1)){
#print "must complete push ptype=$ptype pcol=$pcol pto=$ni ls=$ls\n";
#   we have already started a push, so only our pieces
#     which can complete the push can move
    @a = getAdj($fi);
    foreach $a (@a){
      if ($p[$a] eq " "){ next; }
      ($type,$col) = pieceInfo($p[$a]);
      if ($col eq $t){
        if (isStronger($type, $ptype)){
          if (! isFrozen($a, @p)){
            push(@steps, makeStep($a, $fi, @p));
          }
        }
      }
    }
  }
  else{
#print "regular move\n";
# if the rabbit has reached the last row or if no rabbits
#   remaining then more steps cannot be taken.
# we should not enforce this since the applet allows more
#   steps to be taken after a win.  Also we can pull
#   opponents rabbit in the last row and push it back out
#   before the turn is over.

#    for($i=0;$i<8;$i++){
#      if ($p[$i] eq 'R'){ return @steps; }
#    }
#    for($i=56;$i<64;$i++){
#      if ($p[$i] eq 'r'){ return @steps; }
#    }
#    $pstr = join('', @p);
#    if ($pstr !~ m/[rR]/){ return @steps; }

#   go through all the pieces on the board
    for($i=0;$i<$#p+1;$i++){
      if ($p[$i] eq " "){ next; }
      ($type, $col) = pieceInfo($p[$i]);
      if ($col eq $t){
#       our unfrozen pieces can take steps
        if (! isFrozen($i, @p)){
          $xd = ($t eq "w")?"s":"n";
          if (pieceVal($type) != 1){ $xd = ""; }
          @a = getAdj($i, $xd);
          foreach $a (@a){
            if ($p[$a] eq " "){
              push(@steps, makeStep($i, $a, @p));
            }
          }
        }
      }
      else{
#       this is opponents pieces 
#       see if it can be pulled
#print "check pull for $p[$i] \n";
        $pulled = 0;
        $pulledto = -1;
        if (($pcol eq $t) && ($ls == 0) && isStronger($ptype, $type)){
          @a = getAdj($i);
          foreach $a (@a){
            if ($a == $fi){
              push(@steps, makeStep($i, $fi, @p));
              $pulled = 1;
              $pulledto = $fi;
              last;
            }
          }
#          if ($pulled){ next; }
        }
#       see if it can be pushed
#print "check push for $p[$i] \n";
        if (($stepsLeft >= 2) && isPushable($i, @p)){
          @a = getAdj($i);
          foreach $a (@a){
            if ($p[$a] eq " "){
              if ($a != $pulledto){ # add push only if it cant be pulled here
#if ($type eq 'r'){ print "added push from $i to $a\n"; }
                push(@steps, makeStep($i, $a, @p) . "-"); # use - to mark as a push
              }
            }
          }
        }
      }
    }
  }
#print "** out getSteps\n";
  return @steps;
}

sub isPushable{
  local($i, @p) = @_;
  local($ptype,$pcol,$type,$col,@a,$a);

  ($ptype, $pcol) = pieceInfo($p[$i]);
  @a = getAdj($i);
  foreach $a (@a){
    if ($p[$a] eq " "){ next; }
    ($type, $col) = pieceInfo($p[$a]);
    if (($col ne $pcol) && isStronger($type, $ptype)){
      if (! isFrozen($a, @p)){
        return 1;
      }
    }
  }
  return 0;
}

sub takeSteps{
  local($s, @p) = @_;
  local($type, $col, $fi, $ni, @s);
  
  @s = split(/ /,$s);
  foreach $s (@s){
    ($type,$col, $fi, $ni) = stepInfo($s);
    if ($type eq " "){ next; }
    if ($fi<0){
      $p[$ni] = $type;
    }
    elsif ($ni<0){
      $p[$fi] = " ";
    }
    else{
      $p[$ni] = $p[$fi];
      $p[$fi] = " ";
    }
  }
  return @p;
}

sub takeBackSteps{
  local($s, @p) = @_;
  local($type, $col, $fi, $ni, @s);
  
  @s = split(/ /,$s);
  @s = reverse(@s);
  foreach $s (@s){
    ($type,$col, $fi, $ni) = stepInfo($s);
    if ($type eq " "){ next; }
    if ($ni<0){
      $p[$fi] = $type;
    }
    elsif ($fi<0){
      $p[$ni] = " ";
    }
    else{
      $p[$fi] = $p[$ni];
      $p[$ni] = " ";
    }
  }
  return @p;
}

sub checkTraps{
  local(@p) = @_;
  local(@ts, $ts, @a, $a, $trapped, $type, $col, $atype, $acol, $x, $y);

  @ts = (2*8+2, 5*8+5, 2*8+5, 5*8+2);
  foreach $ts (@ts){
    if ($p[$ts] eq " "){ next; }
    ($type,$col) = pieceInfo($p[$ts]);
    @a = getAdj($ts);
    $trapped = 1;
    foreach $a (@a){
      if ($p[$a] eq " "){ next; }
      ($atype,$acol) = pieceInfo($p[$a]);
      if ($acol eq $col){ $trapped = 0; last; }
    }
    if ($trapped){ return $ts; }
  }
}
  

sub makeStep{
  local($i, $ni, @p) = @_;
  local($z, $x, $y, $nx, $ny, $s, $ti, @np);

  if ($i != -1){
    $y = int($i/8);
    $x = $i%8;
  }
  if ($ni != -1){
    $ny = int($ni/8);
    $nx = $ni%8;
  }
  if ($x == -1){
    $nx =~ tr/01234567/abcdefgh/;
    $ny = 8 - $ny;
    return "$p[$i]$nx$ny";
  }
  if ($ni == -1){
    $x =~ tr/01234567/abcdefgh/;
    $y = 8 - $y;
    return "$p[$i]$x${y}x";
  }
  if ($x == $nx){
    if ($ny < $y){ $z = "n"; }
    if ($ny > $y){ $z = "s"; }
  }
  if ($y == $ny){
    if ($nx < $x){ $z = "w"; }
    if ($nx > $x){ $z = "e"; }
  }
  $x =~ tr/01234567/abcdefgh/;
  $y = 8 - $y;
  $s = "$p[$i]$x$y$z";
  @np = takeSteps($s, @p);
  $ti = checkTraps(@np);
  if ($ti){ $s = "$s " . makeStep($ti, -1, @np); }
  return $s;
}

sub isFrozen{
  local($i, @p) = @_;
  local(@a, $a, $x, $y, $type, $col, $atype, $acol, $frozen);

  if ($p[$i] eq " "){ return 0; }
  ($type,$col) = pieceInfo($p[$i]);
  @a = getAdj($i);
  $frozen = 0;
  foreach $a (@a){
    if ($p[$a] eq " "){ next; }
    ($atype,$acol) = pieceInfo($p[$a]);
    if ($acol eq $col){ return 0; }
    if (isStronger($atype, $type)){ $frozen = 1; }
  }
  return $frozen;
}

sub getAdj{
  local($i, $x) = @_;
  local(@a);

  if ($i>7){ if ($x ne "n"){ push(@a, $i-8); }}
  if ($i<56){ if ($x ne "s"){ push(@a, $i+8); }}
  if ($i%8 != 7){ push(@a, $i+1); }
  if ($i%8 != 0){ push(@a, $i-1); }
  return @a;
}

sub pieceInfo{
  local($t) = @_;

  if ($t eq ""){ $t = " "; }
  $c = ($t =~ m/[a-z]/)?"b":"w";
  if ($t eq " "){ $c = " "; }
#  $t = lc($t);
  return ($t, $c);
}

sub lastStepType{
  local($t, @s) = @_;
  local(@a, $i);
  local($type, $col, $x, $y, $nx, $ny);
  local($ptype, $pcol, $px, $pnx);

# Get rid of step that show trapping; not needed here
  @s = grep(/[^x]$/, @s);

# 0=???? 1=push -1=pull
  if ($#s < 0){ return 0; }
  for($i=0;$i<$#s+1;$i++){
    if ($i == 0){ $a[0] = 0; next; }
    ($type,$col, $x, $nx) = stepInfo($s[$i]);
    if (($x<0) || ($nx<0)){ $a[$i] = $a[$i-1]; next; }
    if ($a[$i-1] != 0){ $a[$i] = 0; next; }
    ($ptype, $pcol, $px, $pnx) = stepInfo($s[$i-1]);
    if ($col ne $t){ # enemy is being moved
      $a[$i] = 0;
      if (($pcol eq $t) && ($px == $nx) && isStronger($ptype,$type)){
        $a[$i] = -1;
      }
    }
    else{
      $a[$i] = 0;
      if ($pcol ne $t){ $a[$i] = 1; }
    }
  }
  return ($a[$#s]);
}

# return 1 if a is stronger than b
sub isStronger{
  local($a, $b) = @_;
  $a = pieceVal($a);
  $b = pieceVal($b);
  return ($a>$b);
}

sub pieceVal{
  local($p) = @_;

  $p = lc($p);
  $p =~ tr/emhdcr/654321/;
  return $p;
}

sub stepInfo{
  local($s) = @_;
  local($t, $c, $x, $y, $z, $i, $ni);

  ($t,$x,$y,$z) = split(//,$s);
  ($t, $c) = pieceInfo($t);
  $x =~ tr/abcdefgh/01234567/;
  $y = 8 - $y;
  $i = 8*$y + $x;
  $ni = $i;
  if ($z eq "n"){ $ni -= 8; }
  if ($z eq "s"){ $ni += 8; }
  if ($z eq "w"){ $ni -= 1; }
  if ($z eq "e"){ $ni += 1; }
  if ($z eq ""){ $i = -1; } # placement of piece
  if ($z eq "x"){ $ni = -1; } # removal of piece
  return ($t, $c, $i, $ni);
}



sub readBoard{
  local($fn) = @_;
  local(*FH, @f, $f);

  open(FH, "<$fn");
  @f = <FH>;
  close FH;

  $f = join('', @f);
  return parsePos($f);
}

sub parsePos{
  local($p) = @_;
  local($a, $b, @b, $t, $s);
  local($c, $t2);

  $p =~ s/\r//gs;
  $p =~ s/\n+/\n/gs;
  $p =~ s/^[ \t\n]//s;

  ($a,$b) = ($p =~ m/^([^\+]*)(\+[\w\W]+)/);
#print "$a\n";
#print " $b\n";

  $a =~ s/\n/ /g;
  ($t, $c, $t2, $s) = ($a =~ m/^ *(w|b)? *(\d+)?(w|b)? *(.*)/);

#print "$t $c $t2 [$s]\n";
  if ($t2){ $t = $t2; }

#print "turn = $t\n";
#  $s =~ s/^\d+(w|b) *//;
  $s =~ s/ +/ /g;
  $s =~ s/^ //;
  $s =~ s/ $//;
#print "steps = $s\n";

  $b =~ s/\n//gs;
#  $b =~ s/^\d//gm;
  ($b) = ($b =~ m/\|(.+)/);
  $b =~ s/ \+\-.*//;
  $b =~ s/ (.)/$1/g;
  $b =~ s/\d//g;
  $b =~ s/\|//g;
  $b =~ s/[xX]/ /g;

#print "b = '$b'\n";
  @b = split(//, $b);
  return ($c, $t, $s, @b);
  
}

sub writeBoard{
  local($fn, $c, $t, $s, @b) = @_;
  local(*FH, $b, $i, $row);

  if ($b[18] eq " "){ $b[18] = "X"; }
  if ($b[21] eq " "){ $b[21] = "X"; }
  if ($b[42] eq " "){ $b[42] = "X"; }
  if ($b[45] eq " "){ $b[45] = "X"; }
  open(FH, ">$fn");
  print FH "$c$t $s\n";
  print FH " +-----------------+\n";
  for($i=0;$i<64;$i++){
    if ($i%8 == 0){
      $row = 8 - $i/8;
      print FH "$row|";
    }
    print FH " $b[$i]";
    if ($i%8 == 7){
      print FH " |\n";
    }
  }
  print FH " +-----------------+\n";
  print FH "   a b c d e f g h\n";
  close FH;
}

1;
