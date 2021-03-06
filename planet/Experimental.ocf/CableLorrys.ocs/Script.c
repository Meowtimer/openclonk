/*--
	Cable Network
	Authors: Randrian, Clonkonaut, Maikel
	
	This scenario's sole purpose is to test the cable system.
	A premade settlement is available for the developer to
	test all aspects of cable networks.
--*/


protected func Initialize()
{
	var workshop = CreateObject(ToolsWorkshop, 835, 360);
	var c1 = CreateObject(CableCrossing, 765, 355);
	var c2 = CreateObject(CableCrossing, 695, 415);
	var c3 = CreateObject(CableCrossing, 585, 415);
	var c4 = CreateObject(CableCrossing, 555, 385);
	var cabin = CreateObject(WoodenCabin, 490, 390);
//	CreateObject(LiftTower, 935, 360);

//	CreateObject(CableLine)->SetConnectedObjects(workshop, c1);
	CreateObject(CableLine)->SetConnectedObjects(c1, c2);
	CreateObject(CableLine)->SetConnectedObjects(c2, c3);
	CreateObject(CableLine)->SetConnectedObjects(c3, c4);
//	CreateObject(CableLine)->SetConnectedObjects(c4, cabin);

	CreateObject(Lorry, 835, 360);

	CreateConstruction(Elevator, 160, 390, NO_OWNER, 100, true)->CreateShaft(150);

	// Forest on the left side of the map, with sawmill.
/*	for (var i = 0; i < 20; i++)
		PlaceVegetation(Tree_Coniferous, 0, 200, 180, 300, 1000 * RandomX(60, 90));
	var sawmill = CreateObject(CableCrossing, 190, 390); // TODO: Replace with sawmill
	sawmill->CreateContents(Wood, 10); //TODO: remove if sawmill exists
	835, 360
	765, 360
	695, 420
	585, 420
	555, 390
	490, 390
	// Foundry to produce metal in the middle of the map.
	var foundry = CreateObject(Foundry, 490, 390);
	var car = foundry->CreateObject(CableLorry);
	car->EngageRail(foundry);
	
	// Chest near the foundry with necessary tools for the player.
	var chest = CreateObject(Chest, 530, 390);
	chest->CreateContents(Dynamite, 4);
	chest->CreateContents(Shovel, 2);
	chest->CreateContents(Hammer, 2);
	chest->CreateContents(CableLorryReel, 2);
	
	// Tool workshop on the little mountain.
	var tools = CreateObject(ToolsWorkshop, 540, 260);
	
	// Crossing on island, connected to sawmill, foundry and tool workshop.
/*	var cross_isle1 = CreateObject(CableCrossing, 320, 390);
	var cross_isle2 = CreateObject(CableCrossing, 360, 390);
	var cross_tools1 = CreateObject(CableCrossing, 450, 290);
	CreateObject(CableLine)->SetConnectedObjects(cross_isle1, sawmill);
	CreateObject(CableLine)->SetConnectedObjects(cross_isle1, cross_isle2);
	CreateObject(CableLine)->SetConnectedObjects(cross_isle2, foundry);
	CreateObject(CableLine)->SetConnectedObjects(cross_isle2, cross_tools1);
	CreateObject(CableLine)->SetConnectedObjects(cross_tools1, tools);*/
	
	// Wooden cabin on the granite.
//	var cabin = CreateObject(WoodenCabin, 850, 360);
	
	// Crossings from foundry into the mines.
/*	var cross_foundry1 = CreateObject(CableCrossing, 560, 390);
	var cross_foundry2 = CreateObject(CableCrossing, 600, 420);
	var cross_mine = CreateObject(CableCrossing, 630, 490);
	CreateObject(CableLine)->SetConnectedObjects(foundry, cross_foundry1);
	CreateObject(CableLine)->SetConnectedObjects(cross_foundry1, cross_foundry2);
	CreateObject(CableLine)->SetConnectedObjects(cross_foundry2, cross_mine);*/
	
	// Crossings from tool workshop to cabin.
/*	var cross_tools2 = CreateObject(CableCrossing, 610, 260);
	var cross_tools3 = CreateObject(CableCrossing, 670, 310);
	var cross_cabin1 = CreateObject(CableCrossing, 760, 360);
	CreateObject(CableLine)->SetConnectedObjects(tools, cross_tools2);
	CreateObject(CableLine)->SetConnectedObjects(cross_tools2, cross_tools3);
	CreateObject(CableLine)->SetConnectedObjects(cross_tools3, cross_cabin1);
	
	// Crossing from cabin to mines.
	var cross_minecabin = CreateObject(CableCrossing, 690, 420);
	CreateObject(CableLine)->SetConnectedObjects(cross_cabin1, cross_minecabin);
	CreateObject(CableLine)->SetConnectedObjects(cross_minecabin, cross_mine);
	
	// Crossings from mine central to ore mine.
	var cross_ore1 = CreateObject(CableCrossing, 550, 550);
	var cross_ore2 = CreateObject(CableCrossing, 470, 620);
	var cross_ore3 = CreateObject(CableCrossing, 370, 650);
	var cross_ore4 = CreateObject(CableCrossing, 250, 600);
	var cross_ore5 = CreateObject(CableCrossing, 200, 570);
	CreateObject(CableLine)->SetConnectedObjects(cross_mine, cross_ore1);
	CreateObject(CableLine)->SetConnectedObjects(cross_ore1, cross_ore2);
	CreateObject(CableLine)->SetConnectedObjects(cross_ore2, cross_ore3);
	CreateObject(CableLine)->SetConnectedObjects(cross_ore3, cross_ore4);
	CreateObject(CableLine)->SetConnectedObjects(cross_ore4, cross_ore5);
	
	// A departure into the firestone mine.
	var cross_sulph1 = CreateObject(CableCrossing, 290, 670);
	var cross_sulph2 = CreateObject(CableCrossing, 250, 700);
	CreateObject(CableLine)->SetConnectedObjects(cross_ore3, cross_sulph1);
	CreateObject(CableLine)->SetConnectedObjects(cross_sulph1, cross_sulph2);
	
	// A departure into the coal mine.
	var cross_coal1 = CreateObject(CableCrossing, 470, 710);
	CreateObject(CableLine)->SetConnectedObjects(cross_ore3, cross_coal1);
	
	// Some resources are already available in the mines.
	cross_ore5->CreateContents(Ore, 10);
	cross_coal1->CreateContents(Coal, 10);
	cross_sulph2->CreateContents(Firestone, 10);*/

/*	DrawMaterialQuad("Earth-earth", 235, 250, 265, 250, 265, 255, 235, 255);
	DrawMaterialQuad("Earth-earth", 185, 250, 215, 250, 215, 255, 185, 255);
	DrawMaterialQuad("Earth-earth", 235, 300, 265, 300, 265, 305, 235, 305);
	DrawMaterialQuad("Earth-earth", 285, 250, 315, 250, 315, 255, 285, 255);
	DrawMaterialQuad("Earth-earth", 235, 200, 265, 200, 265, 205, 235, 205);
	var c1 = CreateObject(CableCrossing, 250, 240);
	var c2 = CreateObject(CableCrossing, 200, 240);
	var c3 = CreateObject(CableCrossing, 250, 290);
	var c4 = CreateObject(CableCrossing, 300, 240);
	var c5 = CreateObject(CableCrossing, 250, 190);
	CreateObject(CableLine)->SetConnectedObjects(c1, c2);
	CreateObject(CableLine)->SetConnectedObjects(c1, c3);
	CreateObject(CableLine)->SetConnectedObjects(c1, c4);
	CreateObject(CableLine)->SetConnectedObjects(c1, c5);
	CreateObject(CableLine)->SetConnectedObjects(c2, c3);
	CreateObject(CableLine)->SetConnectedObjects(c3, c4);
	CreateObject(CableLine)->SetConnectedObjects(c4, c5);
	CreateObject(CableLine)->SetConnectedObjects(c5, c2);
	CreateObject(CableLorry)->EngageRail(c2);*/

	// Structures to cable stations, shouldn't this be automatic?
	// TODO: implement a method.

	// Initial message for the user.
//	Log("Give the lorry commands with SetDestination(target);");
//	Log("target can be the number of the crossing or a pointer to the crossing");
//	Log("The network already has been set up, Activate the producers to stations and watch.");
	return;
}

protected func InitializePlayer(int plr)
{
	// No FOW here.
	SetFoW(false, plr);
	JoinPlayer(plr);
	return;
}

protected func RelaunchPlayer(int plr)
{
	var clonk = CreateObject(Clonk, 0, 0, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	JoinPlayer(plr);
	return;
}

private func JoinPlayer(int plr)
{
	var clonk = GetCrew(plr);
	clonk->DoEnergy(100000);
	clonk->SetPosition(510, 370);
	return;
}