/*
	Copyright 2020 David "Alemarius Nexus" Lerch

	This file is part of RemoteMono.

	RemoteMono is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	RemoteMono is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with RemoteMono.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "pch.h"
#include "config.h"

using namespace remotemono;


// This file implements the RedRunner sample using the RemoteMono helper classes (remotemono/helper). These classes are a
// more high-level, C++-friendly wrapper around the direct methods of the RMonoAPI class. The helper classes allow you
// to write much more compact and readable code.
// However, they are not a complete replacement for the direct API provided by RMonoAPI. The helper classes only aim to
// provide a nicer interface for the most common things you usually do with RemoteMono (creating objects, getting/setting
// fields and properties, calling methods, etc.). If you want to do something that the helper classes don't let you do
// directly, you can do just that thing using RMonoAPI directly. The helper classes are automatically converted to the
// low-level types (e.g. RMonoObject to RMonoObjectPtr, RMonoMethod to RMonoMethodPtr, etc.), so you can use helper class
// objects where RMonoAPI expects their low-level versions.



struct TestContext
{
	RMonoHelperContext* h;
	RMonoAPI* mono;

	// Mono Assemblies
	RMonoAssemblyPtr ass;
	RMonoAssemblyPtr ueAss;
	RMonoAssemblyPtr ueUiAss;

	// Mono Assembly Images
	RMonoImagePtr img;
	RMonoImagePtr ueImg;
	RMonoImagePtr ueUiImg;

	// Unity classes
	RMonoClass gameObjCls;
	RMonoClass componentCls;
	RMonoClass textCls;
	RMonoClass transformCls;
	RMonoClass rectTransformCls;
	RMonoClass vector2Cls;
	RMonoClass vector3Cls;
	RMonoClass colorCls;

	// RedRunner classes
	RMonoClass gameManagerCls;
	RMonoClass audioManagerCls;
	RMonoClass mainCharCls;

	// RedRunner Objects
	RMonoObject gameManagerObj;
	RMonoObject audioManagerObj;
	RMonoObject mainCharObj;
};

static bool ShutdownRequested = false;



void SetupTestContext(TestContext& ctx)
{
	ctx.h = new RMonoHelperContext(ctx.mono);

	RMonoAPI& mono = *ctx.mono;
	RMonoHelperContext& h = *ctx.h;

	RMonoLogInfo("Using RemoteMono helper classes.");

	// Gather assemblies
	ctx.ass = mono.assemblyLoaded("Assembly-CSharp");
	ctx.ueAss = mono.assemblyLoaded("UnityEngine");
	ctx.ueUiAss = mono.assemblyLoaded("UnityEngine.UI");

	// Gather assembly images
	ctx.img = mono.assemblyGetImage(ctx.ass);
	ctx.ueImg = mono.assemblyGetImage(ctx.ueAss);
	ctx.ueUiImg = mono.assemblyGetImage(ctx.ueUiAss);

	// Gather UnityEngine classes
	ctx.gameObjCls = h.classFromName(ctx.ueImg, "UnityEngine", "GameObject");
	ctx.componentCls = h.classFromName(ctx.ueImg, "UnityEngine", "Component");
	ctx.textCls = h.classFromName(ctx.ueUiImg, "UnityEngine.UI", "Text");
	ctx.transformCls = h.classFromName(ctx.ueImg, "UnityEngine", "Transform");
	ctx.rectTransformCls = h.classFromName(ctx.ueImg, "UnityEngine", "RectTransform");
	ctx.vector2Cls = h.classFromName(ctx.ueImg, "UnityEngine", "Vector2");
	ctx.vector3Cls = h.classFromName(ctx.ueImg, "UnityEngine", "Vector3");
	ctx.colorCls = h.classFromName(ctx.ueImg, "UnityEngine", "Color");

	// Gather RedRunner classes
	ctx.gameManagerCls = h.classFromName(ctx.img, "RedRunner", "GameManager");
	ctx.audioManagerCls = h.classFromName(ctx.img, "RedRunner", "AudioManager");

	// Gather RedRunner singleton objects
	ctx.gameManagerObj = ctx.gameManagerCls.property("Singleton").get();
	ctx.audioManagerObj = ctx.audioManagerCls.property("Singleton").get();

	// Get the main character (an RedRunner.RedCharacter instance)
	ctx.mainCharObj = ctx.gameManagerObj.field("m_MainCharacter").get();

	// Get the main characters class (we could also have gotten it explicitly like all the other classes above)
	ctx.mainCharCls = ctx.mainCharObj.getClass();
}


void IncreaseMovementSpeed(TestContext& ctx)
{
	RMonoAPI& mono = *ctx.mono;

	// Set a bunch of fields on the main character
	//ctx.mainCharObj.field("m_JumpStrength").set(15.0f);
	ctx.mainCharObj.field("m_MaxRunSpeed").set(15.0f);
	ctx.mainCharObj.field("m_RunSpeed").set(10.0f);
	ctx.mainCharObj.field("m_WalkSpeed").set(7.5f);
	ctx.mainCharObj.field("m_RunSmoothTime").set(1.5f);
}


void SetupDoubleJump(TestContext& ctx)
{
	RMonoAPI& mono = *ctx.mono;

	// This just sets the maximum distance-from-ground at which the character is still considered on the ground to some large
	// value. Thus, the character is effectively always "on the ground", allowing multi-jump.
	// This does not work when the character is over water, because then the distance to the ground is (probably) infinite.
	ctx.mainCharObj.field("m_GroundCheck").get().field("m_RayDistance").set(1000.0f);
}


void PlaySound(TestContext& ctx, const std::string& soundMethodName)
{
	RMonoAPI& mono = *ctx.mono;

	// Just call one of the methods in RedRunner.AudioManager
	ctx.audioManagerObj.method(soundMethodName)(ctx.vector3Cls.allocObject());
}


void GetMainCharacterPosition(TestContext& ctx, float& x, float& y)
{
	RMonoAPI& mono = *ctx.mono;

	auto mainCharPos = ctx.mainCharObj.property("transform").get().property("position").get();

	// In more recent Unity versions, Vector3.x/y/z are properties, but the RedRunner version I'm testing on uses an older
	// version, where they are still fields.
	if (mainCharPos.property("x")) {
		x = mainCharPos.property("x").get<float>();
		y = mainCharPos.property("y").get<float>();
	} else {
		x = mainCharPos.field("x").get<float>();
		y = mainCharPos.field("y").get<float>();
	}
}


RMonoObjectPtr AddCanvasText (
		TestContext& ctx,
		const std::string& text,
		int fontSize,
		float x, float y,
		float width, float height,
		float anchorX, float anchorY
) {
	RMonoAPI& mono = *ctx.mono;
	RMonoHelperContext& h = *ctx.h;

	// NOTE: The code below creates a new GameObject with a Text component and sets up the necessary parameters for it to
	// be displayed properly. It can be relatively hard to read the corresponding RemoteMono calls, so the equivalent
	// C# code is displayed in the comments above it.

	// GameObject scoreTextObj = GameObject.Find("Score Text");
	// Text scoreText = (Text) scoreTextObj.GetComponent(typeof(Text));
	// Font font = scoreText.font;
	auto scoreTextObj = ctx.gameObjCls.methodDesc(":Find(string)")(h.str("Score Text"));
	auto scoreText = scoreTextObj.methodDesc(":GetComponent(Type)")(ctx.textCls.typeObject());
	auto font = scoreText.property("font").get();

	// GameObject newTextObj = new GameObject("RemoteMonoTestText");
	// Transform inGameScreenTrf = GameObject.Find("In-Game Screen").transform;
	// newTextObj.transform.SetParent(inGameScreenTrf);
	auto newTextObj = ctx.gameObjCls.newObject(h.str("RemoteMonoTestText"));
	auto inGameScreenTrf = ctx.gameObjCls.methodDesc(":Find(string)")(h.str("In-Game Screen")).property("transform").get();
	newTextObj.property("transform").get().methodDesc(":SetParent(Transform)")(inGameScreenTrf);

	// RectTransform trf = (RectTransform) newTextObj.AddComponent(typeof(RectTransform));
	auto trf = newTextObj.methodDesc(":AddComponent(Type)")(ctx.rectTransformCls.typeObject());

	// trf.anchoredPosition = new Vector2(x, y);
	// trf.anchorMin = new Vector2(anchorX, anchorY);
	// trf.anchorMax = new Vector2(anchorX, anchorY);
	// trf.localScale = new Vector2(1.0f, 1.0f);
	// trf.sizeDelta = new Vector2(width, height);
	trf.property("anchoredPosition").set(ctx.vector2Cls.newObject(x, y));
	trf.property("anchorMin").set(ctx.vector2Cls.newObject(anchorX, anchorY));
	trf.property("anchorMax").set(ctx.vector2Cls.newObject(anchorX, anchorY));
	trf.property("localScale").set(ctx.vector2Cls.newObject(1.0f, 1.0f));
	trf.property("sizeDelta").set(ctx.vector2Cls.newObject(width, height));

	// Text newText = newTextObj.AddComponent<Text>();
	auto newText = newTextObj.methodDesc(":AddComponent(Type)")(ctx.textCls.typeObject());

	// newText.text = text;
	// newText.fontSize = fontSize;
	// newText.font = font;
	newText.property("text").set(h.str(text));
	newText.property("fontSize").set(fontSize);
	newText.property("font").set(font);

	// newText.color = Color.red;
	newText.property("color").set(ctx.colorCls.property("red").get());

	return newTextObj;
}


void SetCanvasText(TestContext& ctx, RMonoObjectPtr textObj, const std::string& text)
{
	RMonoAPI& mono = *ctx.mono;
	RMonoHelperContext& h = *ctx.h;

	// Objects of helper classes can be constructed from handles like this (helper context must always be passed).
	RMonoObject htextObj(&h, textObj);

	auto textComp = htextObj.methodDesc(":GetComponent(Type)")(ctx.textCls.typeObject());
	textComp.property("text").set(h.str(text));
}
