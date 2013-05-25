/* Spray can */

local last_x, last_y, last_ldx, last_ldy;
local paint_col;

static SprayCan_last_col;

protected func Construction()
{
	SetPaintCol(SprayCan_last_col++);
	return true;
}

func SetPaintCol(int idx)
{
	idx %= 5;
	if (idx) SetGraphics(Format("%d",idx)); else SetGraphics();
	paint_col = Format("Tunnel-Paint%s", ["Red", "Green", "Teal", "Yellow", "White"][idx]);
	return true;
}

// Impact sound
func Hit()
{
	Sound("GeneralHit?");
}

// Item activation
func ControlUseStart(object clonk, int x, int y)
{
	x += GetX(); y += GetY();
	last_x = x; last_y = y;
	last_ldx=last_ldy=0;
	var r = Random(90), wdt = 2;
	var ldx = Sin(r, wdt), ldy = Cos(r, wdt);
	DrawMaterialQuad(paint_col, x-ldx,y-ldy, x-ldy,y+ldx, x+ldx,y+ldy, x+ldy,y-ldx, DMQ_Bridge);
	SetAction("Spraying");
	return true;
}

func HoldingEnabled() { return true; }

func ControlUseHolding(object clonk, int new_x, int new_y)
{
	new_x += GetX(); new_y += GetY();
	if (new_x==last_x && new_y == last_y) return true;
	var wdt = 2;
	var dx=new_x-last_x, dy=new_y-last_y;
	var d = Distance(dx,dy);
	var ldx = dy*wdt/d, ldy = -dx*wdt/d;
	if (!last_ldx && !last_ldy) { last_ldx=ldx; last_ldy=ldy; }
	DrawMaterialQuad(paint_col, last_x-last_ldx,last_y-last_ldy, last_x+last_ldx,last_y+last_ldy, new_x+ldx,new_y+ldy, new_x-ldx,new_y-ldy, DMQ_Bridge);
	last_x = new_x; last_y = new_y;
	last_ldx = ldx; last_ldy = ldy;
	return true;
}

public func ControlUseStop(object clonk, int x, int y)
{
	SetAction("Idle");
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	SetAction("Idle");
	return true;
}

local ActMap = {
	Spraying = {
		Prototype = Action,
		FacetBase = 1,
		Length = 1,
		Delay = 1,
		Name = "Spraying",
		Sound = "SprayCan",
		NextAction = "Spraying",
	}
};


local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Rebuy = true;