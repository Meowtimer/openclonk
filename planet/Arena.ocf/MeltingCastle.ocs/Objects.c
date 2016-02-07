/* Automatically created objects file */

func InitializeObjects()
{
	CreateObject(Rule_KillLogs, 50, 50);

	CreateObject(Rule_Gravestones, 50, 50);

	CreateObject(Goal_Melee, 50, 50);

	ItemSpawn->Create(Firestone,407,389);
	ItemSpawn->Create(Firestone,423,389);
	ItemSpawn->Create(PowderKeg,669,261);
	ItemSpawn->Create(Bread,407,258);
	ItemSpawn->Create(IronBomb,441,389);

	var Chest001 = CreateObjectAbove(Chest, 1047, 359);

	CreateObjectAbove(Idol, 313, 254);

	var WoodenBridge001 = CreateObjectAbove(WoodenBridge, 1126, 372);
	WoodenBridge001->SetCategory(C4D_StaticBack);
	var WoodenBridge002 = CreateObjectAbove(WoodenBridge, 513, 282);
	WoodenBridge002->SetCategory(C4D_StaticBack);

	CreateObjectAbove(Goal_Flag, 507, 180);

	CreateObjectAbove(Catapult, 728, 431);
	CreateObjectAbove(Catapult, 627, 269);

	var Cannon001 = CreateObjectAbove(Cannon, 692, 253);
	Cannon001->SetRDir(3);
	CreateObjectAbove(Cannon, 511, 179);

	CreateObjectAbove(Airship, 369, 269);

	Chest001->CreateContents(Boompack);

	Chest001->CreateContents(IronBomb, 2);

	Chest001->CreateContents(Bread);

	Chest001->CreateContents(BombArrow);

	Chest001->CreateContents(Bow);
	return true;
}
