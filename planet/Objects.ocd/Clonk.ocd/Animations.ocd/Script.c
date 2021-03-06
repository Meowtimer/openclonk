/*--
	Animation effects of the clonk
	Authors: Randrian

	Is responsible for the management of all the animations of the clonk. It containes:
	* Turn
	* Animation Manager
	* Eyes
	* Walk
	* Scale
	* Jump
	* Hangle
	* Swim
	* Roll and kneel
	* Digging
	* Throwing
	* Dead
	* Tumble
	* Riding
	* Pushing
	* HangOnto

--*/


local lAnim; // proplist containing all the specific variables. "Pseudo-Namespace"

func Construction()
{
	lAnim = 
	{
		turnType = nil,
		turnSpecial = nil,
		turnForced = nil,
		backwards = nil,
		backwardsSpeed = nil,
		closedEyes = nil,
		rollDir = nil,
		rollLength = nil
	};
	
	_inherited(...);
}

/*--
	Turn

	Turn behavoir of the clonk. There are two turn types. Type 0 always rotated the clonk 25° to the screen. Type 1 views the clonk always from the side.
	The clonk turns around, when he changes dir.
--*/


// todo, implement, make carryheavy decreasing it
static const Clonk_TurnTime = 18;

func SetMeshTransformation() { return _inherited(...); }
func UpdateAttach() { return _inherited(...); }
func GetHandAction() { return _inherited(...); }
func DoThrow() { return _inherited(...); }

local ActMap;

func SetTurnForced(int dir)
{
	lAnim.turnForced = dir+1;
}

func FxIntTurnStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	effect.dir = GetDirection();
	var iTurnPos = 0;
	if(effect.dir == COMD_Right) iTurnPos = 1;

	effect.curr_rot = 24;
	effect.rot = 25;
	effect.turn_type = -1;
	SetTurnType(0);
}

func FxIntTurnTimer(pTarget, effect, iTime)
{
	// Check wether the clonk wants to turn (Not when he wants to stop)
	var iRot = effect.rot;
	if( (effect.dir != GetDirection() && (GetAction() != "Jump") || this->~IsAiming()) || effect.turn_type != lAnim.turnType) 
	{
		effect.dir = GetDirection();
		if(effect.dir == COMD_Right)
		{
			if(lAnim.turnType == 0)
				iRot = 180-25;
			if(lAnim.turnType == 1)
				iRot = 180;
		}
		else
		{
			if(lAnim.turnType == 0)
				iRot = 25;
			if(lAnim.turnType == 1)
				iRot = 0;
		}
		// Save new ComDir
		effect.dir = GetDirection();
		effect.turn_type = lAnim.turnType;
		// Notify effects
//		ResetAnimationEffects();
	}
	if(iRot != effect.curr_rot)
	{
		effect.curr_rot += BoundBy(iRot-effect.curr_rot, -18, 18);
		SetMeshTransformation(Trans_Rotate(effect.curr_rot, 0, 1, 0), 0);
	}
	effect.rot = iRot;
	return;
}

public func UpdateTurnRotation()
{
	var iEff = GetEffect("IntTurn", this);
	iEff.turn_type = -1;
}

public func GetTurnPhase()
{
	var iEff = GetEffect("IntTurn", this);
	var iRot = iEff.curr_rot;
	if(lAnim.turnType == 0)
		return (iRot-25)*100/130;
	if(lAnim.turnType == 1)
		return iRot*100/180;
}

func SetTurnType(iIndex, iSpecial)
{
	if(iSpecial != nil && iSpecial != 0)
	{
		if(iSpecial == 1) // Start a turn that is forced to the clonk and overwrites the normal action's turntype
			lAnim.turnSpecial = 1;
		if(iSpecial == -1) // Reset special turn (here the iIndex is ignored)
		{
			lAnim.turnSpecial = 0;
			SetTurnType(lAnim.turnType);
			return;
		}
	}
	else
	{
		// Standart turn? Save and do nothing if we are blocked
		lAnim.turnType = iIndex;
		if(lAnim.turnSpecial) return;
	}
	return;
}

func GetDirection()
{
	// Are we forced to a special direction?
	if(lAnim.turnForced)
	{
		if(lAnim.turnForced == 1) return COMD_Left;
		if(lAnim.turnForced == 2) return COMD_Right;
	}
	// Get direction from ComDir
	if(GetAction() != "Scale")
	{
		if(ComDirLike(GetComDir(), COMD_Right)) return COMD_Right;
		else if(ComDirLike(GetComDir(), COMD_Left)) return COMD_Left;
	}
	// if ComDir hasn't a direction, use GetDir
	if(GetDir()==DIR_Right) return COMD_Right;
	else return COMD_Left;
}

/*--
	Animation Manager

	Animations can be replaced. E.g. while aiming with the bow the normal walk animation gets replaced by a bow walk animation.
--*/

local PropAnimations;
local ActualReplace;

public func ReplaceAction(string action, byaction)
{
	if(PropAnimations == nil) PropAnimations = CreatePropList();
	if(byaction == nil || byaction == 0)
	{
		SetProperty(action, nil, PropAnimations);
		ResetAnimationEffects();
		return true;
	}
/*	if(GetAnimationLength(byaction) == nil)
	{
		Log("ERROR: No animation %s in Definition %s", byaction, GetID()->GetName());
		return false;
	}*/
	if(GetType(byaction) == C4V_Array)
	{
		var old = GetProperty(action, PropAnimations);
		SetProperty(action, byaction, PropAnimations);
		if(GetType(old) == C4V_Array)
		{
			if(ActualReplace == nil) return true;
			if(old[0] == byaction[0] && old[1] == byaction[1])
			{
				var i = 0;
				for (test in ActualReplace)
				{
					if(test && test[0] == action)
						break;
					i++;
				}
				if(i < GetLength(ActualReplace))
					SetAnimationWeight(ActualReplace[i][1], Anim_Const(byaction[2]));
				return true;
			}
		}
	}
	else SetProperty(action, byaction, PropAnimations);
//	if(ActualReplace != nil)
//		SetAnimationWeight(ActualReplace, Anim_Const(byaction[2]));
	ResetAnimationEffects();
	return true;
}

public func ResetAnimationEffects()
{
	if(GetEffect("IntWalk", this))
		EffectCall(this, GetEffect("IntWalk", this), "Reset");
	if(GetAction() == "Jump")
		StartJump();
}

public func PlayAnimation(string animation, int index, array position, array weight, int sibling)
{
	if(!ActualReplace) ActualReplace = [];
	ActualReplace[index] = nil;
	if(PropAnimations != nil)
		if(GetProperty(animation, PropAnimations) != nil)
		{
			var replacement = GetProperty(animation, PropAnimations);
			if(GetType(replacement) == C4V_Array)
			{
				var animation1 = inherited(replacement[0], index, position, weight);
				var animation2 = inherited(replacement[1], index, position, Anim_Const(500), animation1);
				var animationKnot = animation2 + 1;
				ActualReplace[index] = [animation, animationKnot];
				SetAnimationWeight(animationKnot, Anim_Const(replacement[2]));
				return animation1;
			}
			else
				animation = GetProperty(animation, PropAnimations);
		}
	return inherited(animation, index, position, weight, sibling, ...);
}

public func GetAnimationLength(string animation)
{
	var replacement;
	if(PropAnimations != nil)
		if(replacement = GetProperty(animation, PropAnimations))
		{
			if(GetType(replacement) == C4V_Array)
				animation = replacement[0];
			else
				animation = replacement;
		}
	return inherited(animation, ...);
}

/*--
	Eyes

	The clonk automatically closed his eyes every now and then. With CloseEyes(number) the eyes can be opened and shut. If the counter is > 0 the eyes are shut if not they are open. This is also used e.g. by the tumbling animation. While tumbling the clonk's eyes are shut.
--*/

func FxIntEyesTimer(target, effect, time)
{
	if(!Random(4))
		AddEffect("IntEyesClosed", this, 10, 6, this);
}

func FxIntEyesClosedStart(target, effect, tmp)
{
	CloseEyes(1);
}

func FxIntEyesClosedStop(target, effect, reason, tmp)
{
	CloseEyes(-1);
}

func CloseEyes(iCounter)
{
	StopAnimation(GetRootAnimation(3));
	lAnim.closedEyes += iCounter;
	if(lAnim.closedEyes >= 1)
		PlayAnimation("CloseEyes" , 3, Anim_Linear(0, 0, GetAnimationLength("CloseEyes")/2, 3, ANIM_Hold), Anim_Const(1000));
	else
		PlayAnimation("CloseEyes" , 3, Anim_Linear(GetAnimationLength("CloseEyes")/2, GetAnimationLength("CloseEyes")/2, GetAnimationLength("CloseEyes"), 3, ANIM_Remove), Anim_Const(1000));
}

/*--
	Walk

	The Clonk adjusts his animation acording to his velocity: Stand, Walk and Run. It he is in a building he uses "Inside" insead of "Stand". For walking while aiming in the different direction a speed for walking bachwards can be set.
	While standing and doing nothing the clonk starts an idle animation every now and then.
--*/

/* Walking backwards */
func SetBackwardsSpeed(int value)
{
	lAnim.backwardsSpeed = value;
	UpdateBackwardsSpeed();
}

func UpdateBackwardsSpeed()
{
	if(GetComDir() != GetDirection() && lAnim.backwards != 1 && lAnim.backwardsSpeed != nil)
	{
		AddEffect("IntWalkBack", this, 1, 0, this, 0, lAnim.backwardsSpeed);
		lAnim.backwards = 1;
	}
	if( (GetComDir() == GetDirection() && lAnim.backwards == 1) || lAnim.backwardsSpeed == nil)
	{
		RemoveEffect("IntWalkBack", this);
		lAnim.backwards = nil;
	}
}

func FxIntWalkBackStart(pTarget, effect, fTmp, iValue)
{
	if(iValue == nil) iValue = 84;
	pTarget->PushActionSpeed("Walk", iValue);
}

func FxIntWalkBackStop(pTarget, effect)
{
	pTarget->PopActionSpeed("Walk");
}

/* Walk */

static const Clonk_WalkInside = "Inside";
static const Clonk_WalkStand = "Stand";
static const Clonk_WalkWalk  = "Walk";
static const Clonk_WalkRun   = "Run";
static Clonk_IdleActions;

func StartWalk()
{
	if(Clonk_IdleActions == nil)
		Clonk_IdleActions = [["IdleLookAround", 60], ["IdleHandwatch", 100], ["IdleScratch", 70], ["IdleStrech", 100], ["IdleShoe", 120], ["IdleShoeSole", 200], ["IdleHandstrech", 100]];
	if(!GetEffect("IntWalk", this))
		AddEffect("IntWalk", this, 1, 1, this);
}

func StopWalk()
{
	if(GetAction() != "Walk") RemoveEffect("IntWalk", this);
}

func GetCurrentWalkAnimation()
{
	if(Contained())
	{
		if(Contained()->GetCategory() & C4D_Structure)
		{
			return Clonk_WalkInside;
		}
		return;
	}
	else SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,1000,5000), Trans_Rotate(70,0,1,0)), this);
	var velocity = Distance(0,0,GetXDir(),GetYDir());
	if(velocity < 1) return Clonk_WalkStand;
	if(velocity < 10) return Clonk_WalkWalk;
	return Clonk_WalkRun;
}

func Footstep()
{
	if (GetMaterialVal("DigFree", "Material", GetMaterial(0,10)) == 0)
		Sound("StepHard?");
	else
	{
		var dir = Sign(GetXDir());
		var clr = GetAverageTextureColor(GetTexture(0,10));
		var particles =
		{
			Prototype = Particles_Dust(),
			R = (clr >> 16) & 0xff,
			G = (clr >> 8) & 0xff,
			B = clr & 0xff,
		};
		CreateParticle("Dust", PV_Random(dir * -2, dir * -1), 8, PV_Random(dir * 2, dir * 1), PV_Random(-2, -3), PV_Random(36, 2 * 36), particles, 5);
		Sound("StepSoft?");
	}
}


func GetWalkAnimationPosition(string anim, int pos)
{
	var dir = -1;
	if(GetDirection() == COMD_Right) dir = +1;
	if(PropAnimations != nil)
		if(GetProperty(Format("%s_Position", anim), PropAnimations))
		{
			var length = GetAnimationLength(anim), replacement;
			if(replacement = GetProperty(anim, PropAnimations))
			{
				// at this point /replacement/ may contain an array of two animations that signal a merge
				// in that case, just take the first one..
				if(GetType(replacement) == C4V_Array)
					replacement = replacement[0];
				length = GetAnimationLength(replacement);
			}
			return Anim_X(pos, 0, length, GetProperty(Format("%s_Position", anim), PropAnimations)*dir);
		}
	// TODO: Choose proper starting positions, depending on the current
	// animation and its position: For Stand->Walk or Stand->Run, start
	// with a frame where one of the clonk's feets is on the ground and
	// the other one is in the air. For Walk->Run and Run->Walk, fade to
	// a state where its feets are at a similar position (just taking
	// over previous animation's position might work, using
	// GetAnimationPosition()). Walk->Stand is arbitrary I guess.
	// First parameter of Anim_Linear/Anim_AbsX is initial position.
	// Movement synchronization might also be tweaked somewhat as well.
	if(anim == Clonk_WalkInside)
		return Anim_Const(0);
	if(anim == Clonk_WalkStand)
		return Anim_Linear(pos, 0, GetAnimationLength(anim), 35, ANIM_Loop);
	else if(anim == Clonk_WalkWalk)
		return Anim_X(pos, 0, GetAnimationLength(anim), 20*dir);
	else if(anim == Clonk_WalkRun)
		return Anim_X(pos, 0, GetAnimationLength(anim), 50*dir);
}

func FxIntWalkStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	// Always start in Stand for now... should maybe fade properly from previous animation instead
	var anim = "Stand";  //GetCurrentWalkAnimation();
	effect.animation_name = anim;
	effect.animation_id = PlayAnimation(anim, 5, GetWalkAnimationPosition(anim), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	effect.idle_animation_time = 0;

	effect.idle_time = 0; // Idle counter
	effect.idle_offset = Random(300); // Random offset for idle time
	// Update carried items
	UpdateAttach();
	// Set proper turn
	SetTurnType(0);
}

func FxIntWalkTimer(pTarget, effect)
{
	// Test Waterlevel
	if(InLiquid() && GBackLiquid(0, -5) && !Contained())
	{
		SetAction("Swim");
		if(GetComDir() == COMD_Left)
			SetComDir(COMD_UpLeft);
		else if(GetComDir() == COMD_Right)
			SetComDir(COMD_UpRight);
		else if(GetComDir() != COMD_Down && GetComDir() != COMD_DownLeft && GetComDir() != COMD_DownRight)
			SetComDir(COMD_Up);
		return;
	}
	if(lAnim.backwardsSpeed != nil)
		UpdateBackwardsSpeed();
	if(effect.idle_animation_time)
	{
		effect.idle_animation_time--;
		if(effect.idle_animation_time == 0)
			effect.animation_name = nil;
	}
	var anim = GetCurrentWalkAnimation();
	if(anim != effect.animation_name)
	{
		effect.animation_name = anim;
		effect.idle_time = 0;
		effect.animation_id = PlayAnimation(anim, 5, GetWalkAnimationPosition(anim, 0), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	}
	// The clonk has to stand, not making a pause animation yet and not doing other actions with the hands (e.g. loading the bow)
	else if(anim == Clonk_WalkStand && !GetHandAction())
	{
		if (effect.footstop_time) effect.footstep_time = 0;
		if(!effect.idle_animation_time)
		{
			effect.idle_time++;
			if(effect.idle_time > 300+effect.idle_offset)
			{
				effect.idle_time = 0;
				effect.idle_offset = Random(300);
				var rand = Random(GetLength(Clonk_IdleActions));
				PlayAnimation(Clonk_IdleActions[rand][0], 5, Anim_Linear(0, 0, GetAnimationLength(Clonk_IdleActions[rand][0]), Clonk_IdleActions[rand][1], ANIM_Remove), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
				effect.idle_animation_time = Clonk_IdleActions[rand][1]-5;
			}
		}
	}
	else
	{
		effect.idle_time = 0;
		if(effect.idle_animation_time)
		{
			effect.animation_name = nil;
			effect.idle_animation_time = 0;
		}
		if (anim == Clonk_WalkRun)
		{
			effect.footstep_time++;
			if (effect.footstep_time == 12)
			{
				Footstep();
				effect.footstep_time = 0;
			}
		}
		else if(effect.footstep_time) effect.footstep_time = 0;
	}
}

func FxIntWalkReset(pTarget, effect)
{
	effect.animation_name = nil;
}

func StartStand()
{
	PlayAnimation(Clonk_WalkStand, 5, GetWalkAnimationPosition(Clonk_WalkStand), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(0);
}

/*--
	Scale

	During scaling the clonk adjusts his rotation to the ground.. When he is a the top, he uses a extra animation. When the wall doesn't have a platform for the feet he just scales using his arms.
--*/

func StartScale()
{
	if(!GetEffect("IntScale", this))
		AddEffect("IntScale", this, 1, 1, this);
	// Set proper turn type
	SetTurnType(1);
	// Update carried items
	UpdateAttach();
}

func StopScale()
{
	if(GetAction() != "Scale") RemoveEffect("IntScale", this);
}

func CheckScaleTop()
{
	// Test whether the clonk has reached a top corner
	// That is, the leg vertices are the only ones attached to the wall
	if(GBackSolid(-3+6*GetDir(),-3) || GBackSolid(-5+10*GetDir(),2)) return false;
	return true;
}

func FxIntScaleStart(target, effect, tmp)
{
	if(tmp) return;
	effect.animation_id = PlayAnimation("Scale", 5, Anim_Y(0, GetAnimationLength("Scale"), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	effect.animation_mode = 0;
}

func FxIntScaleTimer(target, number, time)
{
	if(GetAction() != "Scale") return;
	// When the clonk reaches the top play an extra animation
	if(CheckScaleTop())
	{
		// If the animation is not already set
		var dist = 0;
		while(!(GBackSolid(-3+6*GetDir(),dist-3) || GBackSolid(-5+10*GetDir(),dist+2)) && dist < 8) dist++;
		dist *= 100;
		// add the fractional part of the position (dist counts in the opposite direction of y)
		dist -= GetY(100)-GetY()*100;
		if(number.animation_mode != 1)
		{
			number.animation_id = PlayAnimation("ScaleTop", 5, Anim_Const(GetAnimationLength("ScaleTop")*dist/800), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			number.animation_mode = 1;
		}
		this.dist = dist;
		SetAnimationPosition(number.animation_id, Anim_Const(GetAnimationLength("ScaleTop")*dist/800));
		// The animation's graphics has to be shifet a bit to adjust to the clonk movement
		var pos = GetAnimationPosition(number.animation_id);
		SetScaleRotation(0, 0, 0, 0, 0, 1);
	}
	else if(!GBackSolid(-10+20*GetDir(), 8))
	{
		if(number.animation_mode != 2)
		{
			var pos = GetAnimationPosition(number.animation_id);
			number.animation_id = PlayAnimation("ScaleHands" , 5, Anim_Y(pos, GetAnimationLength("ScaleHands"), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			number.animation_id2 = PlayAnimation("ScaleHands2", 5, Anim_Y(pos, GetAnimationLength("ScaleHands2"), 0, 15), Anim_Const(1000), number.animation_id);
			number.animation_id2++;
			number.animation_mode = 2;
		}
		SetAnimationWeight(number.animation_id2, Anim_Const(Cos(time*2, 500)+500));
		SetScaleRotation(0);
	}
	// If not play the normal scale animation
	else if(number.animation_mode != 0)
	{
		if(number.ScheduleStop)
		{
			SetComDir(COMD_Stop);
			number.ScheduleStop = 0;
		}
		var pos = 0;
		if(number.animation_mode == 2) pos = GetAnimationPosition(number.animation_id);
		number.animation_id = PlayAnimation("Scale", 5, Anim_Y(0, GetAnimationLength("Scale"), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		number.animation_mode = 0;
		SetScaleRotation(0);
	}
	if(number.animation_mode == 0)
	{
		var x, x2;
		var y = -7, y2 = 8;
		var dir = -1+2*GetDir();
		for(x = 0; x < 10; x++)
			if(GBackSolid(x*dir, y)) break;
		for(x2 = 0; x2 < 10; x2++)
			if(GBackSolid(x2*dir, y2)) break;
		var angle = Angle(x2, y2, x, y)*dir;
		var mid = (x+x2)*1000/2 - 5000 - this.Off;
		this.TestAngle = angle;
		this.TestMid = mid;
		SetScaleRotation(angle, mid*dir);
	}
}

func FxIntScaleRotTimer(target, eff, time)
{
	eff.oldR += BoundBy(eff.r-eff.oldR, -3, 3);
	eff.oldX += BoundBy(eff.xoff-eff.oldX, -500, 500);
	eff.oldY += BoundBy(eff.yoff-eff.oldY, -500, 500);
	var turnx = -1000;
	var turny = 10000;
	SetMeshTransformation(Trans_Mul(Trans_Translate(eff.oldX-turnx, eff.oldY-turny), Trans_Rotate(eff.oldR,0,0,1), Trans_Translate(turnx, turny)), 1);
}

func SetScaleRotation (int r, int xoff, int yoff, int rotZ, int turny, int instant) {
	if(r < -180) r += 360;
	if(r > 180) r -= 360;
	// set matrix values
	var turnx = -1000;
	var turny = 10000;
	if(instant)
	{
		RemoveEffect("IntScaleRot", this);
		SetMeshTransformation(Trans_Mul(Trans_Translate(xoff-turnx, yoff-turny), Trans_Rotate(r,0,0,1), Trans_Translate(turnx, turny), Trans_Rotate(rotZ, 0, 1, 0)), 1);
	}
	else
	{
		var eff = GetEffect("IntScaleRot", this);
		if(!eff)
			eff = AddEffect("IntScaleRot", this, 1, 1, this);
		eff.r = r;
		eff.xoff = xoff;
		eff.yoff = yoff;
	}
}

func FxIntScaleStop(target, number, reason, tmp)
{
	if(tmp) return;
	// Set the animation to stand without blending! That's cause the animation of Scale moves the clonkmesh wich would result in a stange blend moving the clonk around while blending
/*	if(number.animation_mode == 1) PlayAnimation(Clonk_WalkStand, 5, GetWalkAnimationPosition(Clonk_WalkStand), Anim_Const(1000));
	// Finally stop if the user has scheduled a stop
	if(number.ScheduleStop) SetComDir(COMD_Stop);*/
	// and reset the transform
	SetScaleRotation(0);
//	SetObjDrawTransform(1000, 0, 0, 0, 1000, 0);
}

/*--
	Jump

	Different jump animations for different situations.
--*/

func StartJump()
{
	//which leg to kick off with?
	var side = "R";
	if(Random(2)) side = "L";

	//Normal forward jump
	if(Abs(GetXDir()) >= 1)
	PlayAnimation(Format("Jump.%s",side), 5, Anim_Linear(0, 0, GetAnimationLength("Jump.L"), 8*5, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	//Walk kick jump
	if(GetEffect("WallKick",this))
	{
		SetAction("WallJump");
		var side = "L";
		if(GetDir() == DIR_Left) side = "R";
		PlayAnimation(Format("JumpWall.%s", side), 5, Anim_Linear(0, 0, GetAnimationLength("JumpWall.L"), 8*5, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	}
	//Upwards jump
	else if(GetXDir() == 0)
	{
		PlayAnimation(Format("JumpUp.%s", side), 5, Anim_Linear(0, 0, GetAnimationLength("JumpUp.L"), 8*5, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	}

	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(0);
	//Dive jump (only if not aiming)
	if(!this->~IsAiming())
	{
		var flight = SimFlight(AbsX(GetX()), AbsY(GetY()), GetXDir()*2, GetYDir()*2, 25); //I have no clue why the dirs must be doubled... but it seems to fix it
		if(GBackLiquid(flight[0] - GetX(), flight[1] - GetY()) && GBackLiquid(flight[0] - GetX(), flight[1] + GetDefHeight() / 2 - GetY()))
		{
			PlayAnimation("JumpDive", 5, Anim_Linear(0, 0, GetAnimationLength("JumpDive"), 60, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			return 1;
		}
	}

	if(!GetEffect("Fall", this))
		AddEffect("Fall",this,1,1,this);
	RemoveEffect("WallKick",this);
}

func FxFallEffect(string new_name, object target)
{
	// reject more than one fall effects.
	if(new_name == "Fall") return -1;
}

func FxFallTimer(object target, effect, int timer)
{
	//falling off ledges without jumping results in fall animation
	if(timer == 2 && GetYDir() > 1)
	{
		PlayAnimation("FallShort", 5, Anim_Linear(0, 0, GetAnimationLength("FallShort"), 8*3, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	}
	if(timer == 2 && GetYDir() < 1)
	{
		Sound("Rustle?");
	}

	if(GetYDir() > 55 && GetAction() == "Jump")
	{
		PlayAnimation("FallLong", 5, Anim_Linear(0, 0, GetAnimationLength("FallLong"), 8*3, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		return -1;
	}
	if(GetAction() != "Jump")
		return -1;
}

/*--
	Hangle

	Adjust the speed sinusoidal. Plays two different stand animations according to the position the clonk stops.
--*/

/* Replaces the named action by an instance with a different speed */
func PushActionSpeed(string action, int n)
{
	if (ActMap == this.Prototype.ActMap)
		ActMap = { Prototype = this.Prototype.ActMap };
	ActMap[action] = { Prototype = ActMap[action], Speed = n };
	if (this.Action == ActMap[action].Prototype)
		this.Action = ActMap[action];
}

/* Resets the named action to the previous one */
func PopActionSpeed(string action, int n) {
	// FIXME: This only works if PushActionSpeed and PopActionSpeed are the only functions manipulating the ActMap
	if (this.Action == ActMap[action])
		this.Action = ActMap[action].Prototype;
	ActMap[action] = ActMap[action].Prototype;
}

func StartHangle()
{
/*	if(Clonk_HangleStates == nil)
		Clonk_HangleStates = ["HangleStand", "Hangle"];*/
	if(!GetEffect("IntHangle", this))
		AddEffect("IntHangle", this, 1, 1, this);
	// Set proper turn type
	SetTurnType(1);
	// Update carried items
	UpdateAttach();
}

func StopHangle()
{
	if(GetAction() != "Hangle") RemoveEffect("IntHangle", this);
}

func FxIntHangleStart(pTarget, effect, fTmp)
{
	effect.hangle_speed = ActMap.Hangle.Speed;
	PushActionSpeed("Hangle", effect.hangle_speed);
	if(fTmp) return;

	// is_moving: whether the clonk is currently moving or not (<=> current animation is Hangle or HangleStand)
	// request_stop: Player requested the clonk to stop
	// facing_front: Whether the HangleStand animation is shown front-facing or back-facing

	effect.animation_id = PlayAnimation("HangleStand", 5, Anim_Linear(0, 0, 2000, 100, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

}

func FxIntHangleStop(pTarget, effect, iReasonm, fTmp)
{
	PopActionSpeed("Hangle");
	if(fTmp) return;
	// Delayed stop request
	if (effect.request_stop) SetComDir(COMD_Stop);
}

func FxIntHangleTimer(pTarget, effect, iTime)
{
	// (TODO: Instead of effect.is_moving we should be able
	// to query the current animation... maybe via a to-be-implemented
	// GetAnimationName() engine function.

	// If we are currently moving
	if(effect.is_moving)
	{
		// Use a cosine-shaped movement speed (the clonk only moves when he makes a "stroke")
		var iSpeed = 50-Cos(GetAnimationPosition(effect.animation_id)/10*360*2/1000, 50);
		ActMap.Hangle.Speed = effect.hangle_speed*iSpeed/50;

		// Exec movement animation (TODO: Use Anim_Linear?)
		var position = GetAnimationPosition(effect.animation_id);
		position += (effect.hangle_speed*5/48*1000/(14*2));

		SetAnimationPosition(effect.animation_id, Anim_Const(position % GetAnimationLength("Hangle")));

		// Continue movement, if the clonk still has momentum
		if(GetComDir() == COMD_Stop && iSpeed>10)
		{
			// Make it stop after the current movement
			effect.request_stop = 1;

			if(GetDir())
				SetComDir(COMD_Right);
			else
				SetComDir(COMD_Left);
		}
		// Stop movement if the clonk has lost his momentum
		else if(iSpeed <= 10 && (GetComDir() == COMD_Stop || effect.request_stop))
		{
			effect.request_stop = 0;
			SetComDir(COMD_Stop);

			// and remeber the pose (front or back)
			if(GetAnimationPosition(effect.animation_id) > 2500 && GetAnimationPosition(effect.animation_id) < 7500)
				effect.facing_front = 1;
			else
				effect.facing_front = 0;

			// Change to HangleStand animation
			var begin = 4000*effect.facing_front;
			var end = 2000+begin;
			effect.animation_id = PlayAnimation("HangleStand", 5, Anim_Linear(begin, begin, end, 100, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			effect.is_moving = 0;
		}
	}
	else
	{
		// We are currently not moving
		if(GetComDir() != COMD_Stop)
		{
			// Switch to move
			effect.is_moving = 1;
			// start with frame 100 or from the back hanging pose frame 600
			var begin = 10*(100 + 500*effect.facing_front);
			effect.animation_id = PlayAnimation("Hangle", 5, Anim_Const(begin), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		}
	}
}

/*--
	Swim

	Different animation for swiming and diving. Adjusting the rotation while diving.
--*/

func StartSwim()
{
/*	if(Clonk_SwimStates == nil)
		Clonk_SwimStates = ["SwimStand", "Swim", "SwimDive", "SwimTurn", "SwimDiveTurn", "SwimDiveUp", "SwimDiveDown"];*/
	if(!InLiquid()) return;
	if(!GetEffect("IntSwim", this))
		AddEffect("IntSwim", this, 1, 1, this);
	
	return SetSwimmingVertices(true);
}

func StopSwim()
{
	if(GetAction() != "Swim") RemoveEffect("IntSwim", this);
	
	return SetSwimmingVertices(false);
}

func SetSwimmingVertices(bool is_swimming)
{
	var vtx_list = [[0,2,0], [0,-7,4], [0,9,11], [-2,-3,1], [2,-3,2], [-4,2,1], [4,2,2], [-2,6,1], [2,6,2]];
	if (is_swimming)
		vtx_list = [[0,3,0], [0,-2,4], [0,7,11], [-4,0,1], [4,0,2], [-5,3,1], [5,3,2], [-4,4,1], [4,4,2]];
	for (var i = 0; i < GetVertexNum(); i++)
	{
		SetVertex(i, VTX_X, vtx_list[i][0], 2);
		SetVertex(i, VTX_Y, vtx_list[i][1], 2);
		SetVertex(i, VTX_CNAT, vtx_list[i][2], 2);
	}
	return;
}

func FxIntSwimStart(pTarget, effect, fTmp)
{
	if(fTmp) return;

	effect.animation_name = "SwimStand";
	effect.animation = PlayAnimation("SwimStand", 5, Anim_Linear(0, 0, GetAnimationLength("SwimStand"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

	// Set proper turn type
	SetTurnType(0);
	// Update carried items
	UpdateAttach();
}

func FxIntSwimTimer(pTarget, effect, iTime)
{
	var iSpeed = Distance(0,0,GetXDir(),GetYDir());

	// TODO: Smaller transition time between dive<->swim, keep 15 for swimstand<->swim/swimstand<->dive

	// Play stand animation when not moving
	if(Abs(GetXDir()) < 1 && !GBackSemiSolid(0, -5))
	{
		if (GetContact(-1) & CNAT_Bottom)
		{
			SetAction("Walk");
			return -1;
		}
		if(effect.animation_name != "SwimStand")
		{
			effect.animation_name = "SwimStand";
			effect.animation = PlayAnimation("SwimStand", 5, Anim_Linear(0, 0, GetAnimationLength("SwimStand"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
		}
	}
	// Swimming
	else if(!GBackSemiSolid(0, -5))
	{
		var percent = GetAnimationPosition(GetRootAnimation(5))*200/GetAnimationLength("Swim");
		percent = (percent%100);
		if( percent < 40 )
		{
			if(iTime%5 == 0)
			{
				var phases = PV_Linear(0, 7);
				if (GetDir() == 1) phases = PV_Linear(8, 15);
				var color = GetAverageTextureColor(GetTexture(0, 0));
				var particles =
				{
					Size = 16,
					Phase = phases,
					CollisionVertex = 750,
					OnCollision = PC_Die(),
					R = (color >> 16) & 0xff,
					G = (color >>  8) & 0xff,
					B = (color >>  0) & 0xff,
					Attach = ATTACH_Front,
				};
				CreateParticle("Wave", 0, -4, (RandomX(-5,5)-(-1+2*GetDir())*4)/4, 0, 16, particles);
			}
			Sound("Splash?");
		}
		// Animation speed by X
		if(effect.animation_name != "Swim")
		{
			effect.animation_name = "Swim";
			// TODO: Determine starting position from previous animation
			PlayAnimation("Swim", 5, Anim_AbsX(0, 0, GetAnimationLength("Swim"), 25), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
		}
	}
	// Diving
	else
	{
		if(effect.animation_name != "SwimDive")
		{
			effect.animation_name = "SwimDive";
			// TODO: Determine starting position from previous animation
			effect.animation2 = PlayAnimation("SwimDiveUp", 5, Anim_Linear(0, 0, GetAnimationLength("SwimDiveUp"), 40, ANIM_Loop), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
			effect.animation3 = PlayAnimation("SwimDiveDown", 5, Anim_Linear(0, 0, GetAnimationLength("SwimDiveDown"), 40, ANIM_Loop), Anim_Const(500), effect.animation2);
			effect.animation = effect.animation3 + 1;

			// TODO: This should depend on which animation we come from
			// Guess for SwimStand we should fade from 0, otherwise from 90.
			effect.rot = 90;
		}

		if(iSpeed)
		{
			var iRot = Angle(-Abs(GetXDir()), GetYDir());
			effect.rot += BoundBy(iRot - effect.rot, -4, 4);
		}

		// TODO: Shouldn't weight go by sin^2 or cos^2 instead of linear in angle?
		var weight = 1000*effect.rot/180;
		SetAnimationWeight(effect.animation, Anim_Const(1000 - weight));
	}
}

func GetSwimRotation()
{
	var effect = GetEffect("IntSwim", this);
	if(!effect) return 0;
	return effect.rot*(-1+2*(GetDirection()==COMD_Right));
}

/*--
	Roll and kneel
	
	When the clonk hits the ground a kneel animation of are roll are performed.
--*/

func Hit(int iXSpeed, int iYSpeed)
{
	if(iYSpeed < 450) return;
	if(GetAction() != "Walk") return;

	//Roll :D
	if(GetComDir() == COMD_Right && GetDir() == 1 || GetComDir() == COMD_Left && GetDir() == 0)
	{
		if(Abs(iXSpeed) > 130 && iYSpeed <= 80 * 10)
			SetAction("Roll");
		else
		{
			DoKneel();
			if (GetMaterialVal("DigFree", "Material", GetMaterial(0,10)))
			{
				var clr = GetAverageTextureColor(GetTexture(0,10));
				var particles =
				{
					Prototype = Particles_Dust(),
					R = (clr >> 16) & 0xff,
					G = (clr >> 8) & 0xff,
					B = clr & 0xff,
				};
				CreateParticle("Dust", PV_Random(-4, 4), 8, PV_Random(-3, 3), PV_Random(-2, -4), PV_Random(36, 2 * 36), particles, 12);
			}
		}
	}
	else
	{
		DoKneel();
		if (GetMaterialVal("DigFree", "Material", GetMaterial(0,10)))
		{
			var clr = GetAverageTextureColor(GetTexture(0,10));
			var particles =
			{
				Prototype = Particles_Dust(),
				R = (clr >> 16) & 0xff,
				G = (clr >> 8) & 0xff,
				B = clr & 0xff,
			};
			CreateParticle("Dust", PV_Random(-4, 4), 8, PV_Random(-3, 3), PV_Random(-2, -4), PV_Random(36, 2 * 36), particles, 12);
		}
	}
}

func DoKneel()
{
	var iKneelDownSpeed = 18;

	SetXDir(0);
	SetAction("Kneel");
	Sound("RustleLand");
	PlayAnimation("KneelDown", 5, Anim_Linear(0, 0, GetAnimationLength("KneelDown"), iKneelDownSpeed, ANIM_Remove), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

	ScheduleCall(this, "EndKneel", iKneelDownSpeed, 1);
	return 1;
}

func EndKneel()
{
	if(GetAction() != "Roll") SetAction("Walk");
}

//rollp
func StartRoll()
{	
	SetTurnForced(GetDir());
	Sound("Roll");
	if(GetDir() == 1) lAnim.rollDir = 1;
	else
		lAnim.rollDir = -1;

	lAnim.rollLength = 22;
	PlayAnimation("KneelRoll", 5, Anim_Linear(0, 0, 1500, lAnim.rollLength, ANIM_Remove), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	AddEffect("Rolling", this, 1, 1, this);
}

func FxRollingTimer(object target, int num, int timer)
{
	if(GetContact(-1)) SetXDir(23 * lAnim.rollDir);

	//Hacky fun
	var i = 3;
	while(GBackSolid(lAnim.rollDir, 9) && i != 0)
	{
		SetPosition(GetX(),GetY() - 1);
		i--;
	}

	if(timer > lAnim.rollLength)
	{
		SetAction("Walk");
		SetTurnForced(-1);
		lAnim.rollDir = nil;
		return -1;
	}

	if (GetMaterialVal("DigFree", "Material", GetMaterial(0,10)))
	{
		var clr = GetAverageTextureColor(GetTexture(0,10));
		var dir = GetDir()*2-1;
		
		var particles =
		{
			Prototype = Particles_Dust(),
			R = (clr >> 16) & 0xff,
			G = (clr >> 8) & 0xff,
			B = clr & 0xff,
		};
		CreateParticle("Dust", PV_Random(dir * -2, dir * -1), 8, PV_Random(dir * 2, dir * 1), PV_Random(-2, -5), PV_Random(36, 2 * 36), particles, 6);
	}
}

/*--
	Digging
	
	The effect just is responsible for the sound and the termination of digging.
--*/

func StartDigging()
{
	if(!GetEffect("IntDig", this))
		AddEffect("IntDig", this, 1, 1, this);
}

func StopDigging()
{
	if(GetAction() != "Dig") RemoveEffect("IntDig", this);
}

func FxIntDigStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	PlayAnimation("Dig", 5, Anim_Linear(0, 0, GetAnimationLength("Dig"), 36, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

	// Update carried items
	UpdateAttach();

	// Sound
	Sound("Dig?");

	// Set proper turn type
	SetTurnType(0);
}

func FxIntDigTimer(pTarget, effect, iTime)
{
	if(iTime % 36 == 0)
	{
		Sound("Dig?");
	}
	if( (iTime-18) % 36 == 0 ||  iTime > 35)
	{
		var noDig = 1;
		for(var pShovel in FindObjects(Find_ID(Shovel), Find_Container(this)))
			if(pShovel->IsDigging()) noDig = 0;
		if(noDig)
		{
			SetAction("Walk");
			SetComDir(COMD_Stop);
			return -1;
		}
	}
}

/*--
	Throwing
	
	Throw animation
--*/


// custom throw
public func ControlThrow(object target, int x, int y)
{
	// standard throw after all
	if (!x && !y) return false;
	if (!target) return false;

	var throwAngle = Angle(0,0,x,y);

	// walking (later with animation: flight, scale, hangle?) and hands free
	if ( (GetProcedure() == "WALK" || GetAction() == "Jump" || GetAction() == "Dive")
		&& this->~HasHandAction())
	{
		if (throwAngle < 180) SetDir(DIR_Right);
		else SetDir(DIR_Left);
		//SetAction("Throw");
		this->~SetHandAction(1); // Set hands ocupied
		AddEffect("IntThrow", this, 1, 1, this, 0, target, throwAngle);
		return true;
	}
	// attached
	if (GetProcedure() == "ATTACH")
	{
		//SetAction("RideThrow");
		return DoThrow(target,throwAngle);
	}
	return false;
}

func FxIntThrowStart(target, effect, tmp, targetobj, throwAngle)
{
	var iThrowTime = 16;
	if(tmp) return;
	PlayAnimation("ThrowArms", 10, Anim_Linear(0, 0, GetAnimationLength("ThrowArms"), iThrowTime), Anim_Const(1000));
	effect.targetobj = targetobj;
	effect.angle = throwAngle;
}

func FxIntThrowTimer(target, effect, time)
{
	// cancel throw if object does not exist anymore
	if(!effect.targetobj)
		return -1;
	var iThrowTime = 16;
	if(time == iThrowTime*8/15)
		DoThrow(effect.targetobj, effect.angle);
	if(time >= iThrowTime)
		return -1;
}

func FxIntThrowStop(target, effect, reason, tmp)
{
	if(tmp) return;
	StopAnimation(GetRootAnimation(10));
	this->~SetHandAction(0);
}


/*--
	Dead
	
	Animation on clonk death
--*/

func StartDead()
{
	PlayAnimation("Dead", 5, Anim_Linear(0, 0, GetAnimationLength("Dead"), 20, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(1);
}

/*--
	Tumble
	
	Tumble animation with shut eyes.
--*/

func StartTumble()
{
	if(GetEffect("IntTumble", this)) return;
	// Close eyes
	CloseEyes(1);
	PlayAnimation("Tumble", 5, Anim_Linear(0, 0, GetAnimationLength("Tumble"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(0);
	AddEffect("IntTumble", this, 1, 0);
}

func StopTumble()
{
	if(GetAction() != "Tumble")
	{
		RemoveEffect("IntTumble", this);
		CloseEyes(-1);
	}
}

/*--
	Riding
	
	Riding effect. Makes clonk invisible if the mount attaches the clonk as a mesh (e.g. the boompack)
--*/

public func StartRiding()
{
	if(!GetEffect("IntRiding", this))
		AddEffect("IntRiding", this, 1, 0, this);
}

public func AttachTargetLost()
{
	if(GetEffect("IntRiding", this))
		RemoveEffect("IntRiding", this);
}

public func StopRiding()
{
	if(GetEffect("IntRiding", this))
		RemoveEffect("IntRiding", this);
}

func FxIntRidingStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	var pMount = GetActionTarget();
	if(!pMount) return -1;
	if(pMount->~OnMount(this)) // Notifiy the mount, that the clonk is mounted (it should take care, that the clonk get's attached!
	{
		// if mount has returned true we should be attached
		// So make the clonk object invisible
		effect.vis = GetProperty("Visibility");
		SetProperty("Visibility", VIS_None);
	}
	else effect.vis = -1;
	effect.mount = pMount;
}

func FxIntRidingStop(pTarget, effect, fTmp)
{
	if(fTmp) return;
	if(effect.vis != -1)
		SetProperty("Visibility", effect.vis);

	var pMount = effect.mount;
	if(pMount)
		pMount->~OnUnmount(this);
}

/*--
	Pushing
	
	Just plays the push animation.
--*/

func StartPushing()
{
//	if(GetEffect("IntTumble", this)) return;
	// Close eyes
	PlayAnimation("Push", 5, Anim_AbsX(0, 0, GetAnimationLength("Push"), 20), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(1);
//	AddEffect("IntTumble", this, 1, 0);
}

protected func StopPushing()
{
	return _inherited(...);
}

/*--
	HangOnto
	
	Plays the animation and notifies the target if aborted. Used then the clonk hangs on the grappler's rope
--*/

func StartHangOnto()
{
//	if(GetEffect("IntTumble", this)) return;
	// Close eyes
	PlayAnimation("OnRope", 5, Anim_Linear(0, 0, GetAnimationLength("OnRope"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(1);
//	AddEffect("IntTumble", this, 1, 0);
}

protected func AbortHangOnto()
{
	if (GetActionTarget(0))
		GetActionTarget(0)->~HangOntoLost(this);
	return;
}

/*--
	Eat

	Plays the animation
--*/

func StartEat()
{
	// Nom nom
	PlayAnimation("Eat", 10, Anim_Linear(0,0, GetAnimationLength("Eat"), 45, ANIM_Remove), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
}
