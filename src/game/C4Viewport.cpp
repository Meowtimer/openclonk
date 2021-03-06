/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

/* A viewport to each player */

#include <C4Include.h>
#include <C4Viewport.h>

#include <C4ViewportWindow.h>
#include <C4Console.h>
#include <C4Object.h>
#include <C4FullScreen.h>
#include <C4Stat.h>
#include <C4Player.h>
#include <C4ObjectMenu.h>
#include <C4MouseControl.h>
#include <C4PXS.h>
#include <C4GameMessage.h>
#include <C4GraphicsResource.h>
#include <C4GraphicsSystem.h>
#include <C4Landscape.h>
#include <C4PlayerList.h>
#include <C4GameObjects.h>
#include <C4Network2.h>
#include <C4FoWRegion.h>

void C4Viewport::DropFile(const char* fileName, float x, float y)
{
	Game.DropFile(fileName, GetViewX()+x/Zoom, GetViewY()+y/Zoom);
}

bool C4Viewport::UpdateOutputSize()
{
	if (!pWindow) return false;
	// Output size
	C4Rect rect;

#ifdef WITH_DEVELOPER_MODE
	GtkAllocation allocation;
	gtk_widget_get_allocation(GTK_WIDGET(pWindow->render_widget), &allocation);

	// Use only size of drawing area without scrollbars
	rect.x = allocation.x;
	rect.y = allocation.y;
	rect.Wdt = allocation.width;
	rect.Hgt = allocation.height;
#else
	if (!pWindow->GetSize(&rect)) return false;
#endif
	OutX=rect.x; OutY=rect.y;
	ViewWdt=rect.Wdt; ViewHgt=rect.Hgt;
	ScrollView(0,0);
	// Scroll bars
	ScrollBarsByViewPosition();
	// Reset menus
	ResetMenuPositions=true;
	// update internal GL size
	if (pWindow && pWindow->pSurface)
		pWindow->pSurface->UpdateSize(ViewWdt, ViewHgt);
	// Done
	return true;
}

C4Viewport::C4Viewport()
{
	Default();
}

C4Viewport::~C4Viewport()
{
	Clear();
}

void C4Viewport::Clear()
{
	if (pFoW) { delete pFoW; pFoW = NULL; }
	if (pWindow) { delete pWindow->pSurface; pWindow->Clear(); delete pWindow; pWindow = NULL; }
	Player=NO_OWNER;
	viewX=viewY=0;
	targetViewX=targetViewY=0;
	ViewWdt=ViewHgt=0;
	OutX=OutY=ViewWdt=ViewHgt=0;
	DrawX=DrawY=0;
	viewOffsX = viewOffsY = 0;
}

void C4Viewport::DrawOverlay(C4TargetFacet &cgo, const ZoomData &GameZoom)
{
	if (!Game.C4S.Head.Film || !Game.C4S.Head.Replay)
	{
		// Player info
		C4ST_STARTNEW(PInfoStat, "C4Viewport::DrawOverlay: Player Info")
		DrawPlayerInfo(cgo);
		C4ST_STOP(PInfoStat)
		C4ST_STARTNEW(MenuStat, "C4Viewport::DrawOverlay: Menu")
		DrawMenu(cgo);
		C4ST_STOP(MenuStat)
	}

	// Control overlays (if not film/replay)
	if (!Game.C4S.Head.Film || !Game.C4S.Head.Replay)
	{
		// Mouse control
		if (::MouseControl.IsViewport(this))
		{
			C4ST_STARTNEW(MouseStat, "C4Viewport::DrawOverlay: Mouse")
			::MouseControl.Draw(cgo, GameZoom);
			// Draw GUI-mouse in EM if active
			if (pWindow) ::pGUI->RenderMouse(cgo);
			C4ST_STOP(MouseStat)
		}
	}
}

void C4Viewport::DrawMenu(C4TargetFacet &cgo0)
{
	// Get player
	C4Player *pPlr = ::Players.Get(Player);

	// for menus, cgo is using GUI-syntax: TargetX/Y marks the drawing offset; x/y/Wdt/Hgt marks the offset rect
	C4TargetFacet cgo; cgo.Set(cgo0);
	cgo.X = 0; cgo.Y = 0;
	cgo.Wdt = cgo0.Wdt * cgo0.Zoom; cgo.Hgt = cgo0.Hgt * cgo0.Zoom;
	cgo.TargetX = float(cgo0.X); cgo.TargetY = float(cgo0.Y);
	cgo.Zoom = 1;
	pDraw->SetZoom(cgo.X, cgo.Y, cgo.Zoom);

	// Player eliminated
	if (pPlr && pPlr->Eliminated)
	{
		pDraw->TextOut(FormatString(LoadResStr(pPlr->Surrendered ? "IDS_PLR_SURRENDERED" :  "IDS_PLR_ELIMINATED"),pPlr->GetName()).getData(),
		                           ::GraphicsResource.FontRegular, 1.0, cgo.Surface,cgo.TargetX+cgo.Wdt/2,cgo.TargetY+2*cgo.Hgt/3,0xfaFF0000,ACenter);
		return;
	}

	// Player cursor object menu
	if (pPlr && pPlr->Cursor && pPlr->Cursor->Menu)
	{
		if (ResetMenuPositions) pPlr->Cursor->Menu->ResetLocation();
		// if mouse is dragging, make it transparent to easy construction site drag+drop
		bool fDragging=false;
		if (::MouseControl.IsDragging() && ::MouseControl.IsViewport(this))
		{
			fDragging = true;
			pDraw->ActivateBlitModulation(0x4fffffff);
		}
		// draw menu
		pPlr->Cursor->Menu->Draw(cgo);
		// reset modulation for dragging
		if (fDragging) pDraw->DeactivateBlitModulation();
	}
	// Player menu
	if (pPlr && pPlr->Menu.IsActive())
	{
		if (ResetMenuPositions) pPlr->Menu.ResetLocation();
		pPlr->Menu.Draw(cgo);
	}
	// Fullscreen menu
	if (FullScreen.pMenu && FullScreen.pMenu->IsActive())
	{
		if (ResetMenuPositions) FullScreen.pMenu->ResetLocation();
		FullScreen.pMenu->Draw(cgo);
	}

	// Flag done
	ResetMenuPositions=false;

	// restore Zoom
	pDraw->SetZoom(cgo0.X, cgo0.Y, cgo0.Zoom);
}

void C4Viewport::Draw(C4TargetFacet &cgo0, bool fDrawOverlay)
{

#ifdef USE_CONSOLE
	// No drawing in console mode
	return;
#endif
	C4TargetFacet cgo; cgo.Set(cgo0);
	ZoomData GameZoom;
	GameZoom.X = cgo.X; GameZoom.Y = cgo.Y;
	GameZoom.Zoom = cgo.Zoom;

	if (fDrawOverlay)
	{
		// Draw landscape borders. Only if overlay, so complete map screenshots don't get messed up
		if (BorderLeft)  pDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface,cgo.Surface,DrawX,DrawY,BorderLeft,ViewHgt,-DrawX,-DrawY);
		if (BorderTop)   pDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface,cgo.Surface,DrawX+BorderLeft,DrawY,ViewWdt-BorderLeft-BorderRight,BorderTop,-DrawX-BorderLeft,-DrawY);
		if (BorderRight) pDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface,cgo.Surface,DrawX+ViewWdt-BorderRight,DrawY,BorderRight,ViewHgt,-DrawX-ViewWdt+BorderRight,-DrawY);
		if (BorderBottom)pDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface,cgo.Surface,DrawX+BorderLeft,DrawY+ViewHgt-BorderBottom,ViewWdt-BorderLeft-BorderRight,BorderBottom,-DrawX-BorderLeft,-DrawY-ViewHgt+BorderBottom);
	}

	// Set clippers
	cgo.X += BorderLeft; cgo.Y += BorderTop; cgo.Wdt -= int(float(BorderLeft+BorderRight)/cgo.Zoom); cgo.Hgt -= int(float(BorderTop+BorderBottom)/cgo.Zoom);
	GameZoom.X = cgo.X; GameZoom.Y = cgo.Y;
	cgo.TargetX += BorderLeft/Zoom; cgo.TargetY += BorderTop/Zoom;
	// Apply Zoom
	pDraw->SetZoom(GameZoom);
	pDraw->SetPrimaryClipper(cgo.X,cgo.Y,DrawX+ViewWdt-1-BorderRight,DrawY+ViewHgt-1-BorderBottom);

	last_game_draw_cgo = cgo;

	// --- activate FoW here ---

	// Render FoW only if active for player
	C4FoWRegion* pFoW = NULL;
	if (Player != NO_OWNER)
	{
		C4Player *pPlr = ::Players.Get(Player);
		assert(pPlr != NULL);

		if(pPlr->fFogOfWar) pFoW = this->pFoW;
	}

	// Update FoW
	if (pFoW)
	{
		pFoW->Update(C4Rect(
			int32_t(cgo.TargetX), int32_t(cgo.TargetY),
			cgo.Wdt + 1, cgo.Hgt + 1));

		pFoW->Render();
	}

	pDraw->SetFoW(pFoW);

	C4ST_STARTNEW(SkyStat, "C4Viewport::Draw: Sky")
	::Landscape.Sky.Draw(cgo);
	C4ST_STOP(SkyStat)

	::Objects.Draw(cgo, Player, -2147483647 - 1 /* INT32_MIN */, 0);

	// Draw Landscape
	C4ST_STARTNEW(LandStat, "C4Viewport::Draw: Landscape")
	::Landscape.Draw(cgo, pFoW);
	C4ST_STOP(LandStat)

	// draw PXS (unclipped!)
	C4ST_STARTNEW(PXSStat, "C4Viewport::Draw: PXS")
	::PXS.Draw(cgo);
	C4ST_STOP(PXSStat)

	// draw objects
	C4ST_STARTNEW(ObjStat, "C4Viewport::Draw: Objects")
	::Objects.Draw(cgo, Player, 1, 2147483647 /* INT32_MAX */);
	C4ST_STOP(ObjStat)

	// draw global dynamic particles
	C4ST_STARTNEW(PartStat, "C4Viewport::Draw: Dynamic Particles")
	::Particles.DrawGlobalParticles(cgo);
	C4ST_STOP(PartStat)

	// Draw everything else without FoW
	pDraw->SetFoW(NULL);

	// Draw PathFinder
	if (::GraphicsSystem.ShowPathfinder) Game.PathFinder.Draw(cgo);

	// Draw overlay
	if (!Game.C4S.Head.Film || !Game.C4S.Head.Replay) Game.DrawCursors(cgo, Player);

	// Lights overlay
	if (::GraphicsSystem.ShowLights && pFoW) pFoW->Render(&cgo);

	if (fDrawOverlay)
	{
		// Determine zoom of overlay
		float fGUIZoom = GetGUIZoom();
		// now restore complete cgo range for overlay drawing
		pDraw->SetZoom(DrawX,DrawY, fGUIZoom);
		pDraw->SetPrimaryClipper(DrawX,DrawY,DrawX+(ViewWdt-1),DrawY+(ViewHgt-1));
		C4TargetFacet gui_cgo;
		gui_cgo.Set(cgo0);

		gui_cgo.X = DrawX; gui_cgo.Y = DrawY; gui_cgo.Zoom = fGUIZoom;
		gui_cgo.Wdt = int(float(ViewWdt)/fGUIZoom); gui_cgo.Hgt = int(float(ViewHgt)/fGUIZoom);
		gui_cgo.TargetX = GetViewX(); gui_cgo.TargetY = GetViewY();

		last_gui_draw_cgo = gui_cgo;

		// draw custom GUI objects
		::Objects.ForeObjects.DrawIfCategory(gui_cgo, Player, C4D_Foreground, false);

		// Draw overlay
		C4ST_STARTNEW(OvrStat, "C4Viewport::Draw: Overlay")

		if (Application.isEditor) Console.EditCursor.Draw(cgo);

		DrawOverlay(gui_cgo, GameZoom);

		// Game messages
		C4ST_STARTNEW(MsgStat, "C4Viewport::DrawOverlay: Messages")
		pDraw->SetZoom(0, 0, 1.0);
		::Messages.Draw(gui_cgo, cgo, Player);
		C4ST_STOP(MsgStat)

		// Netstats
		if (::GraphicsSystem.ShowNetstatus)
			::Network.DrawStatus(gui_cgo);

		C4ST_STOP(OvrStat)

	}

	// Remove zoom n clippers
	pDraw->SetZoom(0, 0, 1.0);
	pDraw->NoPrimaryClipper();

}

void C4Viewport::BlitOutput()
{
	if (pWindow)
	{
		C4Rect rtSrc,rtDst;
		rtSrc.x = DrawX; rtSrc.y = DrawY;  rtSrc.Wdt = ViewWdt; rtSrc.Hgt = ViewHgt;
		rtDst.x = OutX;  rtDst.y = OutY;   rtDst.Wdt = ViewWdt; rtDst.Hgt = ViewHgt;
		pWindow->pSurface->PageFlip(&rtSrc, &rtDst);
	}
}

void C4Viewport::Execute()
{
	// Adjust position
	AdjustPosition();
	// Current graphics output
	C4TargetFacet cgo;
	C4Window * w = pWindow;
	if (!w) w = &FullScreen;
	cgo.Set(w->pSurface,DrawX,DrawY,int32_t(ceilf(float(ViewWdt)/Zoom)),int32_t(ceilf(float(ViewHgt)/Zoom)),GetViewX(),GetViewY(),Zoom);
	pDraw->PrepareRendering(w->pSurface);
	// Draw
	Draw(cgo, true);
	// Video record & status (developer mode, first player viewport)
	if (Application.isEditor)
		if (Player==0 && (this==::Viewports.GetViewport((int32_t) 0)))
			::GraphicsSystem.Video.Execute();
	// Blit output
	BlitOutput();
}

/* This method is called whenever the viewport size is changed. Thus, its job 
   is to recalculate the zoom and zoom limits with the new values for ViewWdt
   and ViewHgt. */
void C4Viewport::CalculateZoom()
{
	if(!ZoomInitialized)
		InitZoom();

	C4Player *plr = Players.Get(Player);
	if (plr)
		plr->ZoomLimitsToViewport(this);
	else
		SetZoomLimits(0.8*Min<float>(float(ViewWdt)/GBackWdt,float(ViewHgt)/GBackHgt), 8);

}

void C4Viewport::InitZoom()
{
	C4Player *plr = Players.Get(Player);
	if (plr)
	{
		plr->ZoomToViewport(this, true);
	}
	else
	{
		ZoomTarget = Max<float>(float(ViewWdt)/GBackWdt, 1.0f);
		Zoom = ZoomTarget;
	}

	ZoomInitialized = true;
}

void C4Viewport::ChangeZoom(float by_factor)
{
	ZoomTarget *= by_factor;
	if (ZoomLimitMin && ZoomTarget < ZoomLimitMin) ZoomTarget = ZoomLimitMin;
	if (ZoomLimitMax && ZoomTarget > ZoomLimitMax) ZoomTarget = ZoomLimitMax;
}

void C4Viewport::SetZoom(float to_value, bool direct)
{
	ZoomTarget = to_value;
	if (Player != NO_OWNER || !::Application.isEditor)
	{
		if (ZoomLimitMin && ZoomTarget < ZoomLimitMin) ZoomTarget = ZoomLimitMin;
		if (ZoomLimitMax && ZoomTarget > ZoomLimitMax) ZoomTarget = ZoomLimitMax;
	}
	// direct: Set zoom without scrolling to it
	if (direct) Zoom = ZoomTarget;
}

void C4Viewport::SetZoomLimits(float to_min_zoom, float to_max_zoom)
{
	ZoomLimitMin = to_min_zoom;
	ZoomLimitMax = to_max_zoom;
	if (ZoomLimitMax && ZoomLimitMax < ZoomLimitMin) ZoomLimitMax = ZoomLimitMin;
	ChangeZoom(1); // Constrains zoom to limit.
}

float C4Viewport::GetZoomByViewRange(int32_t size_x, int32_t size_y) const
{
	// set zoom such that both size_x and size_y are guarantueed to fit into the viewport range
	// determine whether zoom is to be calculated by x or by y
	bool zoom_by_y = false;
	if (size_x && size_y)
	{
		zoom_by_y = (size_y * ViewWdt > size_x * ViewHgt);
	}
	else if (size_y)
	{
		// no x size passed - zoom by y
		zoom_by_y = true;
	}
	else
	{
		// 0/0 size passed - zoom to default
		if (!size_x)
			size_x = C4VP_DefViewRangeX * 2;
		zoom_by_y = false;
	}
	// zoom calculation
	if (zoom_by_y)
		return float(ViewHgt) / size_y;
	else
		return float(ViewWdt) / size_x;
}

void C4Viewport::SetZoom(float zoomValue)
{
	Zoom = zoomValue;
	// also set target to prevent zoom from changing back
	ZoomTarget = zoomValue;
}

void C4Viewport::AdjustPosition()
{
	if (ViewWdt == 0 || ViewHgt == 0)
	{
		// zero-sized viewport, possibly minimized editor window
		// don't do anything then
		return;
	}

	float ViewportScrollBorder = fIsNoOwnerViewport ? 0 : float(C4ViewportScrollBorder);
	C4Player *pPlr = ::Players.Get(Player);
	if (ZoomTarget < 0.000001f) CalculateZoom();
	// Change Zoom
	assert(Zoom>0);
	assert(ZoomTarget>0);


	if(Zoom != ZoomTarget)
	{
		float DeltaZoom = Zoom/ZoomTarget;
		if(DeltaZoom<1) DeltaZoom = 1/DeltaZoom;

		// Minimal Zoom change factor
		static const float Z0 = pow(C4GFX_ZoomStep, 1.0f/8.0f);

		// We change zoom based on (logarithmic) distance of current zoom
		// to target zoom. The greater the distance the more we adjust the
		// zoom in one frame. There is a minimal zoom change Z0 to make sure
		// we reach ZoomTarget in finite time.
		float ZoomAdjustFactor = Z0 * pow(DeltaZoom, 1.0f/8.0f);

		if (Zoom < ZoomTarget)
			Zoom = Min(Zoom * ZoomAdjustFactor, ZoomTarget);
		if (Zoom > ZoomTarget)
			Zoom = Max(Zoom / ZoomAdjustFactor, ZoomTarget);
	}
	// View position
	if (PlayerLock && ValidPlr(Player))
	{
		float scrollRange, extraBoundsX, extraBoundsY;

		scrollRange = extraBoundsX = extraBoundsY = 0;

		// target view position (landscape coordinates)
		float targetCenterViewX = fixtof(pPlr->ViewX);
		float targetCenterViewY = fixtof(pPlr->ViewY);

		if (pPlr->ViewMode == C4PVM_Scrolling)
		{
			extraBoundsX = extraBoundsY = ViewportScrollBorder;
		}
		else
		{
			scrollRange = Min(ViewWdt/(10*Zoom),ViewHgt/(10*Zoom));

			// if view is close to border, allow scrolling
			if (targetCenterViewX < ViewportScrollBorder) extraBoundsX = Min<float>(ViewportScrollBorder - targetCenterViewX, ViewportScrollBorder);
			else if (targetCenterViewX >= GBackWdt - ViewportScrollBorder) extraBoundsX = Min<float>(targetCenterViewX - GBackWdt, 0) + ViewportScrollBorder;
			if (targetCenterViewY < ViewportScrollBorder) extraBoundsY = Min<float>(ViewportScrollBorder - targetCenterViewY, ViewportScrollBorder);
			else if (targetCenterViewY >= GBackHgt - ViewportScrollBorder) extraBoundsY = Min<float>(targetCenterViewY - GBackHgt, 0) + ViewportScrollBorder;
		}

		extraBoundsX = Max(extraBoundsX, (ViewWdt/Zoom - GBackWdt)/2 + 1);
		extraBoundsY = Max(extraBoundsY, (ViewHgt/Zoom - GBackHgt)/2 + 1);

		// add mouse auto scroll
		if (pPlr->MouseControl && ::MouseControl.InitCentered && Config.Controls.MouseAutoScroll)
		{
			float strength = Config.Controls.MouseAutoScroll/100.0f;
			targetCenterViewX += strength*(::MouseControl.VpX - ViewWdt/2.0f)/Zoom;
			targetCenterViewY += strength*(::MouseControl.VpY - ViewHgt/2.0f)/Zoom;
		}
		
		// scroll range
		targetCenterViewX = BoundBy(targetCenterViewX, targetCenterViewX - scrollRange, targetCenterViewX + scrollRange);
		targetCenterViewY = BoundBy(targetCenterViewY, targetCenterViewY - scrollRange, targetCenterViewY + scrollRange);
		// bounds
		targetCenterViewX = BoundBy(targetCenterViewX, ViewWdt/Zoom/2 - extraBoundsX, GBackWdt - ViewWdt/Zoom/2 + extraBoundsX);
		targetCenterViewY = BoundBy(targetCenterViewY, ViewHgt/Zoom/2 - extraBoundsY, GBackHgt - ViewHgt/Zoom/2 + extraBoundsY);

		targetViewX = targetCenterViewX - ViewWdt/Zoom/2 + viewOffsX;
		targetViewY = targetCenterViewY - ViewHgt/Zoom/2 + viewOffsY;
		// smooth
		int32_t smooth = BoundBy<int32_t>(Config.General.ScrollSmooth, 1, 50);
		ScrollView((targetViewX - viewX) / smooth, (targetViewY - viewY) / smooth);
	}

	UpdateBordersX();
	UpdateBordersY();

	// NO_OWNER can't scroll
	if (fIsNoOwnerViewport) { viewOffsX=0; viewOffsY=0; }
#ifdef WITH_DEVELOPER_MODE
	//ScrollBarsByViewPosition();
#endif
}

void C4Viewport::CenterPosition()
{
	// center viewport position on map
	// set center position
	SetViewX(GBackWdt/2 + ViewWdt/Zoom/2);
	SetViewY(GBackHgt/2 + ViewHgt/Zoom/2);
}

void C4Viewport::UpdateBordersX()
{
	BorderLeft = int32_t(Max(-GetViewX() * Zoom, 0.0f));
	BorderRight = int32_t(Max(ViewWdt - GBackWdt * Zoom + GetViewX() * Zoom, 0.0f));
}

void C4Viewport::UpdateBordersY()
{
	BorderTop = int32_t(Max(-GetViewY() * Zoom, 0.0f));
	BorderBottom = int32_t(Max(ViewHgt - GBackHgt * Zoom + GetViewY() * Zoom, 0.0f));
}

void C4Viewport::Default()
{
	pWindow=NULL;
	pFoW = NULL;
	Player=0;
	viewX=viewY=0;
	targetViewX=targetViewY=0;
	ViewWdt=ViewHgt=0;
	BorderLeft=BorderTop=BorderRight=BorderBottom=0;
	OutX=OutY=ViewWdt=ViewHgt=0;
	DrawX=DrawY=0;
	Zoom = 1.0;
	ZoomTarget = 0.0;
	ZoomInitialized = false;
	ZoomLimitMin=ZoomLimitMax=0; // no limit
	Next=NULL;
	PlayerLock=true;
	ResetMenuPositions=false;
	viewOffsX = viewOffsY = 0;
	fIsNoOwnerViewport = false;
	last_game_draw_cgo.Default();
	last_gui_draw_cgo.Default();
}

void C4Viewport::DrawPlayerInfo(C4TargetFacet &cgo)
{
	C4Facet ccgo;
	if (!ValidPlr(Player)) return;
	
	// Controls
	DrawPlayerStartup(cgo);
}

bool C4Viewport::Init(int32_t iPlayer, bool fSetTempOnly)
{
	// Fullscreen viewport initialization
	// Set Player
	if (!ValidPlr(iPlayer)) iPlayer = NO_OWNER;
	Player=iPlayer;
	if (!fSetTempOnly) fIsNoOwnerViewport = (iPlayer == NO_OWNER);
	if (Application.isEditor)
	{
		// Console viewport initialization
		// Create window
		pWindow = new C4ViewportWindow(this);
		if (!pWindow->Init(Player))
			return false;
		UpdateOutputSize();
		// Disable player lock on unowned viewports
		if (!ValidPlr(Player)) TogglePlayerLock();
		// Draw
		// Don't call Execute right away since it is not yet guaranteed that
		// the Player has set this as its Viewport, and the drawing routines rely
		// on that.
		//Execute();
	}
	else
	{
		// Owned viewport: clear any flash message explaining observer menu
		if (ValidPlr(iPlayer)) ::GraphicsSystem.FlashMessage("");
	}
	// Initialize FoW
	assert(pFoW == NULL);
	if (::Landscape.pFoW && Player != NO_OWNER)
		pFoW = new C4FoWRegion(::Landscape.pFoW, ::Players.Get(iPlayer));
	return true;
}

extern int32_t DrawMessageOffset;

void C4Viewport::DrawPlayerStartup(C4TargetFacet &cgo)
{
	C4Player *pPlr;
	if (!(pPlr = ::Players.Get(Player))) return;
	if (!pPlr->LocalControl || !pPlr->ShowStartup) return;
	int32_t iNameHgtOff=0;

	// Control
	// unnecessary with the current control sets
	if (pPlr && pPlr->ControlSet)
	{
		C4Facet controlset_facet = pPlr->ControlSet->GetPicture();
		if (controlset_facet.Wdt) controlset_facet.Draw(cgo.Surface,
			    cgo.X+(cgo.Wdt-controlset_facet.Wdt)/2,
			    cgo.Y+cgo.Hgt * 2/3 + DrawMessageOffset,
			    0,0);
		iNameHgtOff=GfxR->fctKeyboard.Hgt;
	}

	// Name
	pDraw->TextOut(pPlr->GetName(), ::GraphicsResource.FontRegular, 1.0, cgo.Surface,
	                           cgo.X+cgo.Wdt/2,cgo.Y+cgo.Hgt*2/3+iNameHgtOff + DrawMessageOffset,
	                           pPlr->ColorDw | 0xff000000, ACenter);
}

void C4Viewport::ScrollView(float byX, float byY)
{
	SetViewX(viewX + byX);
	SetViewY(viewY + byY);
}

void C4Viewport::SetViewX(float x)
{
	viewX = x;

	if (fIsNoOwnerViewport)
	{
		if(GBackWdt < ViewWdt / Zoom)
		{
			viewX = GBackWdt/2 - ViewWdt / Zoom / 2;
		}
		else
		{
			viewX = BoundBy(x, 0.0f, GBackWdt - ViewWdt / Zoom);
		}
	}

	UpdateBordersX();
}

void C4Viewport::SetViewY(float y)
{
	viewY = y;

	if (fIsNoOwnerViewport)
	{
		if(GBackHgt < ViewHgt / Zoom)
		{
			viewY = GBackHgt/2 - ViewHgt / Zoom / 2;
		}
		else
		{
			viewY = BoundBy(y, 0.0f, GBackHgt - ViewHgt / Zoom);
		}
	}

	UpdateBordersY();
}

void C4Viewport::SetOutputSize(int32_t iDrawX, int32_t iDrawY, int32_t iOutX, int32_t iOutY, int32_t iOutWdt, int32_t iOutHgt)
{
	int32_t deltaWidth = ViewWdt-iOutWdt;
	int32_t deltaHeight = ViewHgt-iOutHgt;
	// update output parameters
	DrawX=iDrawX; DrawY=iDrawY;
	OutX=iOutX; OutY=iOutY;
	ViewWdt=iOutWdt; ViewHgt=iOutHgt;
	// update view position: Remain centered at previous position
	// scrolling the view must be done after setting the new view width and height
	ScrollView(deltaWidth/2, deltaHeight/2);
	CalculateZoom();
	// Reset menus
	ResetMenuPositions=true;
	// player uses mouse control? then clip the cursor
	C4Player *pPlr;
	if ((pPlr=::Players.Get(Player)))
		if (pPlr->MouseControl)
		{
			::MouseControl.UpdateClip();
			// and inform GUI
			::pGUI->SetPreferredDlgRect(C4Rect(iOutX, iOutY, iOutWdt, iOutHgt));
		}
}

void C4Viewport::ClearPointers(C4Object *pObj)
{

}

void C4Viewport::NextPlayer()
{
	C4Player *pPlr; int32_t iPlr;
	if (!(pPlr = ::Players.Get(Player)))
	{
		if (!(pPlr = ::Players.First)) return;
	}
	else if (!(pPlr = pPlr->Next))
		if (Game.C4S.Head.Film && Game.C4S.Head.Replay)
			pPlr = ::Players.First; // cycle to first in film mode only; in network obs mode allow NO_OWNER-view
	if (pPlr) iPlr = pPlr->Number; else iPlr = NO_OWNER;
	if (iPlr != Player) Init(iPlr, true);
}

bool C4Viewport::IsViewportMenu(class C4Menu *pMenu)
{
	// check all associated menus
	// Get player
	C4Player *pPlr = ::Players.Get(Player);
	// Player eliminated: No menu
	if (pPlr && pPlr->Eliminated) return false;
	// Player cursor object menu
	if (pPlr && pPlr->Cursor && pPlr->Cursor->Menu == pMenu) return true;
	// Player menu
	if (pPlr && pPlr->Menu.IsActive() && &(pPlr->Menu) == pMenu) return true;
	// Fullscreen menu (if active, only one viewport can exist)
	if (FullScreen.pMenu && FullScreen.pMenu->IsActive() && FullScreen.pMenu == pMenu) return true;
	// no match
	return false;
}

// C4ViewportList

C4ViewportList Viewports;

C4ViewportList::C4ViewportList():
	FirstViewport(NULL)
{
	ViewportArea.Default();
}
C4ViewportList::~C4ViewportList()
{
}
void C4ViewportList::Clear()
{
	C4Viewport *next;
	while (FirstViewport)
	{
		next=FirstViewport->Next;
		delete FirstViewport;
		FirstViewport=next;
	}
	FirstViewport=NULL;
}

void C4ViewportList::Execute(bool DrawBackground)
{
	// Background redraw
	if (DrawBackground)
		DrawFullscreenBackground();
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next)
	{
		if (cvp->GetWindow())
			cvp->GetWindow()->RequestUpdate();
		else
			cvp->Execute();
	}
}

void C4ViewportList::DrawFullscreenBackground()
{
	for (int i=0, iNum=BackgroundAreas.GetCount(); i<iNum; ++i)
	{
		const C4Rect &rc = BackgroundAreas.Get(i);
		pDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface,FullScreen.pSurface,rc.x,rc.y,rc.Wdt,rc.Hgt,-rc.x,-rc.y);
	}
}

bool C4ViewportList::CloseViewport(C4Viewport * cvp)
{
	if (!cvp) return false;
	/*C4Viewport *next,*prev=NULL;
	for (C4Viewport *cvp2=FirstViewport; cvp2; cvp2=next)
	  {
	  next=cvp2->Next;
	  if (cvp2 == cvp)
	    {
	    delete cvp;
	    StartSoundEffect("CloseViewport");
	    if (prev) prev->Next=next;
	    else FirstViewport=next;
	    }
	  else
	    prev=cvp2;
	  }*/
	// Chop the start of the chain off
	if (FirstViewport == cvp)
	{
		FirstViewport = cvp->Next;
		delete cvp;
		StartSoundEffect("CloseViewport");
	}
	// Take out of the chain
	else for (C4Viewport * prev = FirstViewport; prev; prev = prev->Next)
		{
			if (prev->Next == cvp)
			{
				prev->Next = cvp->Next;
				delete cvp;
				StartSoundEffect("CloseViewport");
			}
		}
	// Recalculate viewports
	RecalculateViewports();
	// Done
	return true;
}
#ifdef USE_WIN32_WINDOWS
C4Viewport* C4ViewportList::GetViewport(HWND hwnd)
{
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next)
		if (cvp->pWindow->hWindow==hwnd)
			return cvp;
	return NULL;
}
#endif
bool C4ViewportList::CreateViewport(int32_t iPlayer, bool fSilent)
{
	// Create and init new viewport, add to viewport list
	int32_t iLastCount = GetViewportCount();
	C4Viewport *nvp = new C4Viewport;
	bool fOkay = nvp->Init(iPlayer, false);
	if (!fOkay) { delete nvp; return false; }
	C4Viewport *pLast;
	for (pLast=FirstViewport; pLast && pLast->Next; pLast=pLast->Next) {}
	if (pLast) pLast->Next=nvp; else FirstViewport=nvp;
	// Recalculate viewports
	RecalculateViewports();
	// Viewports start off at centered position
	nvp->CenterPosition();
	// Action sound
	if (GetViewportCount()!=iLastCount) if (!fSilent)
			StartSoundEffect("CloseViewport");
	return true;
}

void C4ViewportList::ClearPointers(C4Object *pObj)
{
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next)
		cvp->ClearPointers(pObj);
}
bool C4ViewportList::CloseViewport(int32_t iPlayer, bool fSilent)
{
	// Close all matching viewports
	int32_t iLastCount = GetViewportCount();
	C4Viewport *next,*prev=NULL;
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=next)
	{
		next=cvp->Next;
		if (cvp->Player==iPlayer || (iPlayer==NO_OWNER && cvp->fIsNoOwnerViewport))
		{
			delete cvp;
			if (prev) prev->Next=next;
			else FirstViewport=next;
		}
		else
			prev=cvp;
	}
	// Anything was done?
	if (GetViewportCount()!=iLastCount)
	{
		// Recalculate viewports
		RecalculateViewports();
		// Action sound
		if (!fSilent) StartSoundEffect("CloseViewport");
	}
	return true;
}

void C4ViewportList::RecalculateViewports()
{

	// Fullscreen only
	if (Application.isEditor) return;

	// Sort viewports
	SortViewportsByPlayerControl();

	// Viewport area
	int32_t iBorderTop = 0;
	if (Config.Graphics.UpperBoard)
		iBorderTop = C4UpperBoardHeight;
	ViewportArea.Set(FullScreen.pSurface,0,iBorderTop, C4GUI::GetScreenWdt(), C4GUI::GetScreenHgt()-iBorderTop);

	// Redraw flag
	::GraphicsSystem.InvalidateBg();
#ifdef _WIN32
	// reset mouse clipping
	ClipCursor(NULL);
#else
	// StdWindow handles this.
#endif
	// reset GUI dlg pos
	::pGUI->SetPreferredDlgRect(C4Rect(ViewportArea.X, ViewportArea.Y, ViewportArea.Wdt, ViewportArea.Hgt));

	// fullscreen background: First, cover all of screen
	BackgroundAreas.Clear();
	BackgroundAreas.AddRect(C4Rect(ViewportArea.X, ViewportArea.Y, ViewportArea.Wdt, ViewportArea.Hgt));

	// Viewports
	C4Viewport *cvp;
	int32_t iViews = 0;
	for (cvp=FirstViewport; cvp; cvp=cvp->Next) iViews++;
	if (!iViews) return;
	int32_t iViewsH = (int32_t) sqrt(float(iViews));
	int32_t iViewsX = iViews / iViewsH;
	int32_t iViewsL = iViews % iViewsH;
	int32_t cViewH,cViewX,ciViewsX;
	int32_t cViewWdt,cViewHgt,cOffWdt,cOffHgt,cOffX,cOffY;
	cvp=FirstViewport;
	for (cViewH=0; cViewH<iViewsH; cViewH++)
	{
		ciViewsX = iViewsX; if (cViewH<iViewsL) ciViewsX++;
		for (cViewX=0; cViewX<ciViewsX; cViewX++)
		{
			cViewWdt = ViewportArea.Wdt/ciViewsX;
			cViewHgt = ViewportArea.Hgt/iViewsH;
			cOffX = ViewportArea.X;
			cOffY = ViewportArea.Y;
			cOffWdt = cOffHgt = 0;
			if (ciViewsX * cViewWdt < ViewportArea.Wdt)
				cOffX = (ViewportArea.Wdt - ciViewsX * cViewWdt) / 2;
			if (iViewsH * cViewHgt < ViewportArea.Hgt)
				cOffY = (ViewportArea.Hgt - iViewsH * cViewHgt) / 2 + ViewportArea.Y;
			if (Config.Graphics.SplitscreenDividers)
			{
				if (cViewX < ciViewsX - 1) cOffWdt=4;
				if (cViewH < iViewsH - 1) cOffHgt=4;
			}
			int32_t coViewWdt=cViewWdt-cOffWdt;
			int32_t coViewHgt=cViewHgt-cOffHgt;
			C4Rect rcOut(cOffX+cViewX*cViewWdt, cOffY+cViewH*cViewHgt, coViewWdt, coViewHgt);
			cvp->SetOutputSize(rcOut.x,rcOut.y,rcOut.x,rcOut.y,rcOut.Wdt,rcOut.Hgt);
			cvp=cvp->Next;
			// clip down area avaiable for background drawing
			BackgroundAreas.ClipByRect(rcOut);
		}
	}
}

int32_t C4ViewportList::GetViewportCount()
{
	int32_t iResult = 0;
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next) iResult++;
	return iResult;
}

C4Viewport* C4ViewportList::GetViewport(int32_t iPlayer, C4Viewport* pPrev)
{
	for (C4Viewport *cvp=pPrev ? pPrev->Next : FirstViewport; cvp; cvp=cvp->Next)
		if (cvp->Player==iPlayer || (iPlayer==NO_OWNER && cvp->fIsNoOwnerViewport))
			return cvp;
	return NULL;
}

int32_t C4ViewportList::GetAudibility(int32_t iX, int32_t iY, int32_t *iPan, int32_t iAudibilityRadius)
{
	// default audibility radius
	if (!iAudibilityRadius) iAudibilityRadius = C4AudibilityRadius;
	// Accumulate audibility by viewports
	int32_t iAudible=0; *iPan = 0;
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next)
	{
		float distanceToCenterOfViewport = Distance(cvp->GetViewCenterX(),cvp->GetViewCenterY(),iX,iY);
		iAudible = Max( iAudible,
		                BoundBy<int32_t>(100-100*distanceToCenterOfViewport/C4AudibilityRadius,0,100) );
		*iPan += (iX-(cvp->GetViewCenterX())) / 5;
	}
	*iPan = BoundBy<int32_t>(*iPan, -100, 100);
	return iAudible;
}

void C4ViewportList::SortViewportsByPlayerControl()
{

	// Sort
	bool fSorted;
	C4Player *pPlr1,*pPlr2;
	C4Viewport *pView,*pNext,*pPrev;
	do
	{
		fSorted = true;
		for (pPrev=NULL,pView=FirstViewport; pView && (pNext = pView->Next); pView=pNext)
		{
			// Get players
			pPlr1 = ::Players.Get(pView->Player);
			pPlr2 = ::Players.Get(pNext->Player);
			// Swap order
			if (pPlr1 && pPlr2 && pPlr1->ControlSet && pPlr2->ControlSet && ( pPlr1->ControlSet->GetLayoutOrder() > pPlr2->ControlSet->GetLayoutOrder() ))
			{
				if (pPrev) pPrev->Next = pNext; else FirstViewport = pNext;
				pView->Next = pNext->Next;
				pNext->Next = pView;
				pPrev = pNext;
				pNext = pView;
				fSorted = false;
			}
			// Don't swap
			else
			{
				pPrev = pView;
			}
		}
	}
	while (!fSorted);

}

bool C4ViewportList::ViewportNextPlayer()
{
	// safety: switch valid?
	if ((!Game.C4S.Head.Film || !Game.C4S.Head.Replay) && !GetViewport(NO_OWNER)) return false;
	// do switch then
	C4Viewport *vp = GetFirstViewport();
	if (!vp) return false;
	vp->NextPlayer();
	return true;
}

bool C4ViewportList::FreeScroll(C4Vec2D vScrollBy)
{
	// safety: move valid?
	if ((!Game.C4S.Head.Replay || !Game.C4S.Head.Film) && !GetViewport(NO_OWNER)) return false;
	C4Viewport *vp = GetFirstViewport();
	if (!vp) return false;
	// move then (old static code crap...)
	static int32_t vp_vx=0; static int32_t vp_vy=0; static int32_t vp_vf=0;
	int32_t dx=vScrollBy.x; int32_t dy=vScrollBy.y;
	if (Game.FrameCounter-vp_vf < 5)
		{ dx += vp_vx; dy += vp_vy; }
	vp_vx=dx; vp_vy=dy; vp_vf=Game.FrameCounter;
	vp->ScrollView(dx, dy);
	return true;
}

bool C4ViewportList::ViewportZoomOut()
{
	for (C4Viewport *vp = FirstViewport; vp; vp = vp->Next) vp->ChangeZoom(1.0f/C4GFX_ZoomStep);
	return true;
}

bool C4ViewportList::ViewportZoomIn()
{
	for (C4Viewport *vp = FirstViewport; vp; vp = vp->Next) vp->ChangeZoom(C4GFX_ZoomStep);
	return true;
}

void C4ViewportList::MouseMoveToViewport(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
{
	// Pass on to mouse controlled viewport
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next)
		if (::MouseControl.IsViewport(cvp))
			::MouseControl.Move( iButton,
			                     BoundBy<int32_t>(iX-cvp->OutX,0,cvp->ViewWdt-1),
			                     BoundBy<int32_t>(iY-cvp->OutY,0,cvp->ViewHgt-1),
			                     dwKeyParam );
}
