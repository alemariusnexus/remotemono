/*
	Copyright 2020-2021 David "Alemarius Nexus" Lerch

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


// This file implements the RedRunner sample using only the methods in RMonoAPI. RMonoAPI is the most powerful interface in
// RemoteMono because it's the main class through which RemoteMono provides access to the Mono Embedded API: Everything that
// RemoteMono supports can be done on this class.
// However, code that exclusively uses RMonoAPI can be very tedious to write and hard to read, because RMonoAPI is a direct
// equivalent of the original C API, so it's much more verbose than it has to be when coding in C++. That's why you should
// also take a look at main_helpers.cpp, where the same functionality is implemented using RemoteMono's helper classes,
// which is much less annoying to both write and read.



struct TestContext
{
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
	RMonoClassPtr gameObjCls;
	RMonoClassPtr componentCls;
	RMonoClassPtr textCls;
	RMonoClassPtr transformCls;
	RMonoClassPtr rectTransformCls;
	RMonoClassPtr vector2Cls;
	RMonoClassPtr vector3Cls;
	RMonoClassPtr colorCls;

	// RedRunner classes
	RMonoClassPtr gameManagerCls;
	RMonoClassPtr audioManagerCls;
	RMonoClassPtr mainCharCls;

	// RedRunner Objects
	RMonoObjectPtr gameManagerObj;
	RMonoObjectPtr audioManagerObj;
	RMonoObjectPtr mainCharObj;
};

static bool ShutdownRequested = false;



void SetupTestContext(TestContext& ctx)
{
	RMonoAPI& mono = *ctx.mono;

	RMonoLogInfo("Using RemoteMono direct API (RMonoAPI).");

	// Gather assemblies
	ctx.ass = mono.assemblyLoaded("Assembly-CSharp");
	ctx.ueAss = mono.assemblyLoaded("UnityEngine");
	ctx.ueUiAss = mono.assemblyLoaded("UnityEngine.UI");

	// Gather assembly images
	ctx.img = mono.assemblyGetImage(ctx.ass);
	ctx.ueImg = mono.assemblyGetImage(ctx.ueAss);
	ctx.ueUiImg = mono.assemblyGetImage(ctx.ueUiAss);

	// Gather UnityEngine classes
	ctx.gameObjCls = mono.classFromName(ctx.ueImg, "UnityEngine", "GameObject");
	ctx.componentCls = mono.classFromName(ctx.ueImg, "UnityEngine", "Component");
	ctx.textCls = mono.classFromName(ctx.ueUiImg, "UnityEngine.UI", "Text");
	ctx.transformCls = mono.classFromName(ctx.ueImg, "UnityEngine", "Transform");
	ctx.rectTransformCls = mono.classFromName(ctx.ueImg, "UnityEngine", "RectTransform");
	ctx.vector2Cls = mono.classFromName(ctx.ueImg, "UnityEngine", "Vector2");
	ctx.vector3Cls = mono.classFromName(ctx.ueImg, "UnityEngine", "Vector3");
	ctx.colorCls = mono.classFromName(ctx.ueImg, "UnityEngine", "Color");

	// Gather RedRunner classes
	ctx.gameManagerCls = mono.classFromName(ctx.img, "RedRunner", "GameManager");
	ctx.audioManagerCls = mono.classFromName(ctx.img, "RedRunner", "AudioManager");

	// Gather RedRunner singleton objects
	ctx.gameManagerObj = mono.propertyGetValue(mono.classGetPropertyFromName(ctx.gameManagerCls, "Singleton"));
	ctx.audioManagerObj = mono.propertyGetValue(mono.classGetPropertyFromName(ctx.audioManagerCls, "Singleton"));

	// Get the main character (an RedRunner.RedCharacter instance)
	ctx.mainCharObj = mono.fieldGetValue<RMonoObjectPtr>(ctx.gameManagerObj, mono.classGetFieldFromName(ctx.gameManagerCls, "m_MainCharacter"));

	// Get the main characters class (we could also have gotten it explicitly like all the other classes above)
	ctx.mainCharCls = mono.objectGetClass(ctx.mainCharObj);
}


void IncreaseMovementSpeed(TestContext& ctx)
{
	RMonoAPI& mono = *ctx.mono;

	// Set a bunch of fields on the main character
	//mono.fieldSetValue(mainChar, mono.classGetFieldFromName(mainCharCls, "m_JumpStrength"), 15.0f);
	mono.fieldSetValue(ctx.mainCharObj, mono.classGetFieldFromName(ctx.mainCharCls, "m_MaxRunSpeed"), 15.0f);
	mono.fieldSetValue(ctx.mainCharObj, mono.classGetFieldFromName(ctx.mainCharCls, "m_RunSpeed"), 10.0f);
	mono.fieldSetValue(ctx.mainCharObj, mono.classGetFieldFromName(ctx.mainCharCls, "m_WalkSpeed"), 7.5f);
	mono.fieldSetValue(ctx.mainCharObj, mono.classGetFieldFromName(ctx.mainCharCls, "m_RunSmoothTime"), 1.5f);
}


void SetupDoubleJump(TestContext& ctx)
{
	RMonoAPI& mono = *ctx.mono;

	// This just sets the maximum distance-from-ground at which the character is still considered on the ground to some large
	// value. Thus, the character is effectively always "on the ground", allowing multi-jump.
	// This does not work when the character is over water, because then the distance to the ground is (probably) infinite.
	auto groundCheck = mono.fieldGetValue<RMonoObjectPtr>(ctx.mainCharObj, mono.classGetFieldFromName(ctx.mainCharCls, "m_GroundCheck"));
	auto groundCheckCls = mono.objectGetClass(groundCheck);
	mono.fieldSetValue(groundCheck, mono.classGetFieldFromName(groundCheckCls, "m_RayDistance"), 1000.0f);
}


void PlaySound(TestContext& ctx, const std::string& soundMethodName)
{
	RMonoAPI& mono = *ctx.mono;

	// Just call one of the methods in RedRunner.AudioManager
	auto playSoundMethod = mono.classGetMethodFromName(ctx.audioManagerCls, soundMethodName, 1);
	mono.runtimeInvoke(playSoundMethod, ctx.audioManagerObj, {mono.objectNew(ctx.vector3Cls)});
}


void GetMainCharacterPosition(TestContext& ctx, float& x, float& y)
{
	RMonoAPI& mono = *ctx.mono;

	auto mainCharTrf = mono.propertyGetValue(mono.classGetPropertyFromName(ctx.componentCls, "transform"), ctx.mainCharObj);
	auto mainCharPos = mono.propertyGetValue(mono.classGetPropertyFromName(ctx.transformCls, "position"), mainCharTrf);

	auto xProp = mono.classGetPropertyFromName(ctx.vector3Cls, "x");
	auto yProp = mono.classGetPropertyFromName(ctx.vector3Cls, "y");

	auto xField = mono.classGetFieldFromName(ctx.vector3Cls, "x");
	auto yField = mono.classGetFieldFromName(ctx.vector3Cls, "y");

	// In more recent Unity versions, Vector3.x/y/z are properties, but the RedRunner version I'm testing on uses an older
	// version, where they are still fields.
	if (xProp) {
		// Note that we can pass the boxed object mainCharPos directly. Even though mono_property_get_value() takes
		// value-type objects as raw pointers, RemoteMono supports auto-unboxing for parameters of type RMonoVariant.
		x = mono.objectUnbox<float>(mono.propertyGetValue(mono.classGetPropertyFromName(ctx.vector3Cls, "x"), mainCharPos));
		y = mono.objectUnbox<float>(mono.propertyGetValue(mono.classGetPropertyFromName(ctx.vector3Cls, "y"), mainCharPos));
	} else {
		x = mono.fieldGetValue<float>(mainCharPos, xField);
		y = mono.fieldGetValue<float>(mainCharPos, yField);
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

	// Get methods
	auto gameObjFind = mono.methodDescSearchInClass(":Find(string)", false, ctx.gameObjCls);
	auto gameObjGetComponent = mono.methodDescSearchInClass(":GetComponent(Type)", false, ctx.gameObjCls);
	auto gameObjAddComponent = mono.methodDescSearchInClass(":AddComponent(Type)", false, ctx.gameObjCls);
	auto transformSetParent = mono.methodDescSearchInClass(":SetParent(Transform)", false, ctx.transformCls);
	auto vector2Ctor = mono.methodDescSearchInClass(":.ctor(single,single)", false, ctx.vector2Cls);

	// Get properties
	auto gameObjTransformProp = mono.classGetPropertyFromName(ctx.gameObjCls, "transform");

	// NOTE: The code below creates a new GameObject with a Text component and sets up the necessary parameters for it to
	// be displayed properly. It can be relatively hard to read the corresponding RemoteMono calls, so the equivalent
	// C# code is displayed in the comments above it.


	auto vector2New = [&](float x, float y) {
		// return new Vector2(x, y);
		auto vec2 = mono.objectNew(ctx.vector2Cls);
		mono.runtimeInvoke(vector2Ctor, vec2, {x, y});
		return vec2;
	};

	// GameObject scoreTextObj = GameObject.Find("Score Text");
	// Text scoreText = (Text) scoreTextObj.GetComponent(typeof(Text));
	// Font font = scoreText.font;
	auto scoreTextObj = mono.runtimeInvoke(gameObjFind, nullptr, {mono.stringNew("Score Text")});
	auto scoreText = mono.runtimeInvoke(gameObjGetComponent, scoreTextObj, {mono.typeGetObject(mono.classGetType(ctx.textCls))});
	auto font = mono.propertyGetValue(mono.classGetPropertyFromName(ctx.textCls, "font"), scoreText);

	// GameObject newTextObj = new GameObject("RemoteMonoTestText");
	// Transform inGameScreenTrf = GameObject.Find("In-Game Screen").transform;
	// newTextObj.transform.SetParent(inGameScreenTrf);
	auto newTextObj = mono.objectNew(ctx.gameObjCls);
	mono.runtimeInvoke(mono.classGetMethodFromName(ctx.gameObjCls, ".ctor", 1), newTextObj, {mono.stringNew("RemoteMonoTestText")});
	auto inGameScreenTrf = mono.propertyGetValue(gameObjTransformProp, mono.runtimeInvoke(gameObjFind, nullptr, {mono.stringNew("In-Game Screen")}));
	mono.runtimeInvoke(transformSetParent, mono.propertyGetValue(gameObjTransformProp, newTextObj), {inGameScreenTrf});

	// RectTransform trf = (RectTransform) newTextObj.AddComponent(typeof(RectTransform));
	auto trf = mono.runtimeInvoke(gameObjAddComponent, newTextObj, {mono.typeGetObject(mono.classGetType(ctx.rectTransformCls))});

	// trf.anchoredPosition = new Vector2(x, y);
	// trf.anchorMin = new Vector2(anchorX, anchorY);
	// trf.anchorMax = new Vector2(anchorX, anchorY);
	// trf.localScale = new Vector2(1.0f, 1.0f);
	// trf.sizeDelta = new Vector2(width, height);
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.rectTransformCls, "anchoredPosition"), trf, {vector2New(x, y)});
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.rectTransformCls, "anchorMin"), trf, {vector2New(anchorX, anchorY)});
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.rectTransformCls, "anchorMax"), trf, {vector2New(anchorX, anchorY)});
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.rectTransformCls, "localScale"), trf, {vector2New(1.0f, 1.0f)});
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.rectTransformCls, "sizeDelta"), trf, {vector2New(width, height)});

	// Text newText = newTextObj.AddComponent<Text>();
	auto newText = mono.runtimeInvoke(gameObjAddComponent, newTextObj, {mono.typeGetObject(mono.classGetType(ctx.textCls))});

	// newText.text = text;
	// newText.fontSize = fontSize;
	// newText.font = font;
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.textCls, "text"), newText, {mono.stringNew(text)});
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.textCls, "fontSize"), newText, {fontSize});
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.textCls, "font"), newText, {font});

	// newText.color = Color.red;
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.textCls, "color"), newText, {
			mono.propertyGetValue(mono.classGetPropertyFromName(ctx.colorCls, "red"), nullptr)
			});

	return newTextObj;
}


void SetCanvasText(TestContext& ctx, RMonoObjectPtr textObj, const std::string& text)
{
	RMonoAPI& mono = *ctx.mono;

	auto gameObjGetComponent = mono.methodDescSearchInClass(":GetComponent(Type)", false, ctx.gameObjCls);

	auto textComp = mono.runtimeInvoke(gameObjGetComponent, textObj, {mono.typeGetObject(mono.classGetType(ctx.textCls))});

	mono.propertySetValue(mono.classGetPropertyFromName(ctx.textCls, "text"), textComp, {mono.stringNew(text)});
}
