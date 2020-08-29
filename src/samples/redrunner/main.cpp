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

#include <cstdio>
#include "CLI11.hpp"

using namespace remotemono;


// This is a simple example on how to use RemoteMono. It uses the Unity game RedRunner as a remote. It's best to have
// a copy of RedRunner's source code open when following along with this example. You can get it here:
//
//		https://github.com/BayatGames/RedRunner
//
// To run the example, you have to start RedRunner and enter the game world, then run this example program from the
// console afterwards.



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



void SetupTestContext(TestContext& ctx)
{
	RMonoAPI& mono = *ctx.mono;

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
	mono.runtimeInvoke(playSoundMethod, ctx.audioManagerObj, {mono.objectUnboxRefVariant(mono.objectNew(mono.domainGet(), ctx.vector3Cls))});
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
		auto mainCharPosUnboxed = mono.objectUnboxRefVariant(mainCharPos);

		// Note that we have to pass mainCharPosUnboxed as the object parameter, and NOT mainCharPos. This is because
		// mono_property_get_value() (like mono_runtime_invoke()) takes value-type objects as a raw pointer.
		x = mono.objectUnbox<float>(mono.propertyGetValue(mono.classGetPropertyFromName(ctx.vector3Cls, "x"), mainCharPosUnboxed));
		y = mono.objectUnbox<float>(mono.propertyGetValue(mono.classGetPropertyFromName(ctx.vector3Cls, "y"), mainCharPosUnboxed));
	} else {
		// Unlike mono_property_get_value(), mono_field_get_value() ALWAYS takes a MonoObject* as object parameter, so
		// we have to pass the boxed object. Don't ask me why.
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
		auto vec2 = mono.objectNew(mono.domainGet(), ctx.vector2Cls);
		mono.runtimeInvoke(vector2Ctor, mono.objectUnboxRefVariant(vec2), {x, y});
		return vec2;
	};

	// GameObject scoreTextObj = GameObject.Find("Score Text");
	// Text scoreText = (Text) scoreTextObj.GetComponent(typeof(Text));
	// Font font = scoreText.font;
	auto scoreTextObj = mono.runtimeInvoke(gameObjFind, nullptr, {mono.stringNew("Score Text")});
	auto scoreText = mono.runtimeInvoke(gameObjGetComponent, scoreTextObj, {mono.typeGetObject(mono.domainGet(), mono.classGetType(ctx.textCls))});
	auto font = mono.propertyGetValue(mono.classGetPropertyFromName(ctx.textCls, "font"), scoreText);

	// GameObject newTextObj = new GameObject("RemoteMonoTestText");
	// Transform inGameScreenTrf = GameObject.Find("In-Game Screen").transform;
	// newTextObj.transform.SetParent(inGameScreenTrf);
	auto newTextObj = mono.objectNew(mono.domainGet(), ctx.gameObjCls);
	mono.runtimeInvoke(mono.classGetMethodFromName(ctx.gameObjCls, ".ctor", 1), newTextObj, {mono.stringNew("RemoteMonoTestText")});
	auto inGameScreenTrf = mono.propertyGetValue(gameObjTransformProp, mono.runtimeInvoke(gameObjFind, nullptr, {mono.stringNew("In-Game Screen")}));
	mono.runtimeInvoke(transformSetParent, mono.propertyGetValue(gameObjTransformProp, newTextObj), {inGameScreenTrf});

	// RectTransform trf = (RectTransform) newTextObj.AddComponent(typeof(RectTransform));
	auto trf = mono.runtimeInvoke(gameObjAddComponent, newTextObj, {mono.typeGetObject(mono.domainGet(), mono.classGetType(ctx.rectTransformCls))});

	// trf.anchoredPosition = new Vector2(x, y);
	// trf.anchorMin = new Vector2(anchorX, anchorY);
	// trf.anchorMax = new Vector2(anchorX, anchorY);
	// trf.localScale = new Vector2(1.0f, 1.0f);
	// trf.sizeDelta = new Vector2(width, height);
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.rectTransformCls, "anchoredPosition"), trf,
			{mono.objectUnboxRefVariant(vector2New(x, y))});
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.rectTransformCls, "anchorMin"), trf,
			{mono.objectUnboxRefVariant(vector2New(anchorX, anchorY))});
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.rectTransformCls, "anchorMax"), trf,
			{mono.objectUnboxRefVariant(vector2New(anchorX, anchorY))});
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.rectTransformCls, "localScale"), trf,
			{mono.objectUnboxRefVariant(vector2New(1.0f, 1.0f))});
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.rectTransformCls, "sizeDelta"), trf,
			{mono.objectUnboxRefVariant(vector2New(width, height))});

	// Text newText = newTextObj.AddComponent<Text>();
	auto newText = mono.runtimeInvoke(gameObjAddComponent, newTextObj, {mono.typeGetObject(mono.domainGet(), mono.classGetType(ctx.textCls))});

	// newText.text = text;
	// newText.fontSize = fontSize;
	// newText.font = font;
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.textCls, "text"), newText, {mono.stringNew(text)});
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.textCls, "fontSize"), newText, {fontSize});
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.textCls, "font"), newText, {font});

	// newText.color = Color.red;
	mono.propertySetValue(mono.classGetPropertyFromName(ctx.textCls, "color"), newText, {
			mono.objectUnboxRefVariant(mono.propertyGetValue(mono.classGetPropertyFromName(ctx.colorCls, "red"), nullptr))
			});

	return newTextObj;
}


void SetCanvasText(TestContext& ctx, RMonoObjectPtr textObj, const std::string& text)
{
	RMonoAPI& mono = *ctx.mono;

	auto gameObjGetComponent = mono.methodDescSearchInClass(":GetComponent(Type)", false, ctx.gameObjCls);

	auto textComp = mono.runtimeInvoke(gameObjGetComponent, textObj, {mono.typeGetObject(mono.domainGet(), mono.classGetType(ctx.textCls))});

	mono.propertySetValue(mono.classGetPropertyFromName(ctx.textCls, "text"), textComp, {mono.stringNew(text)});
}



int main(int argc, char** argv)
{

	// ********** COMMAND-LINE PARSING **********

	CLI::App app("redrunner-sample");

	std::string logLevelStr;
	app.add_option("-l,--log-level", logLevelStr, "The logging level. Valid values are: verbose, debug, info, warning, error, none.");

	CLI11_PARSE(app, argc, argv);

	RMonoLogger::LogLevel logLevel = RMonoLogger::LOG_LEVEL_INFO;

	if (logLevelStr == "none") {
		logLevel = RMonoLogger::LOG_LEVEL_NONE;
	} else if (logLevelStr == "verbose") {
		logLevel = RMonoLogger::LOG_LEVEL_VERBOSE;
	} else if (logLevelStr == "debug") {
		logLevel = RMonoLogger::LOG_LEVEL_DEBUG;
	} else if (logLevelStr == "info") {
		logLevel = RMonoLogger::LOG_LEVEL_INFO;
	} else if (logLevelStr == "warning") {
		logLevel = RMonoLogger::LOG_LEVEL_WARNING;
	} else if (logLevelStr == "error") {
		logLevel = RMonoLogger::LOG_LEVEL_ERROR;
	}



	// ********** SETUP **********

	// Setup logging to stdout
	RMonoStdoutLogFunction::getInstance().registerLogFunction();
	RMonoLogger::getInstance().setLogLevel(logLevel);

	std::wstring exeName(L"RedRunner.exe");

	RMonoLogInfo("Attaching BlackBone ...");

	blackbone::Process proc;

	// Find RedRunner process by executable name
	auto pids = proc.EnumByName(exeName);
	if (pids.empty()) {
		RMonoLogError("Target process not found.");
		return 1;
	} else if (pids.size() > 1) {
		RMonoLogError("Multiple target process candidates found.");
		return 1;
	}

	// Attach BlackBone
	NTSTATUS status = proc.Attach(pids[0]);
	if (!NT_SUCCESS(status)) {
		RMonoLogError("Error attaching to target process.");
		return 1;
	}


	RMonoAPI mono(proc);

	// Attach RemoteMono
	RMonoLogInfo("Attaching RemoteMono ...");
	mono.attach();

	TestContext ctx;

	ctx.mono = &mono;



	// ********** USING THE REMOTEMONO API **********

	// Gather frequently used classes and objects
	RMonoLogInfo("Gathering classes and objects ...");
	SetupTestContext(ctx);

	// Increase various movement speed-related values
	RMonoLogInfo("Increasing movement speed ...");
	IncreaseMovementSpeed(ctx);

	// Enable double-jump
	RMonoLogInfo("Enabling double jump (does not work over water) ...");
	SetupDoubleJump(ctx);

	// Add a UI text element to the bottom-left corner
	RMonoLogInfo("Setting up bottom-left text ...");
	auto testTextObj = AddCanvasText (
			ctx,
			"Hello World from RemoteMono!",
			28,
			260.0f, 15.0f,
			500.0f, 50.0f,
			0.0f, 0.0f
			);

	// Play a sound
	RMonoLogInfo("Playing a lovely little sound ...");
	PlaySound(ctx, "PlayChestSound");

	Sleep(3000);

	// Keep querying main character position and display it in the bottom-left text object
	RMonoLogInfo("Will now keep updating bottom-left text with character position.");
	while (true) {
		float x, y;
		GetMainCharacterPosition(ctx, x, y);

		char buf[128];
		sprintf(buf, "Position: %.1f, %.1f", x, y);

		SetCanvasText(ctx, testTextObj, std::string(buf));

		Sleep(50);
	}



	// ********** CLEANUP **********

	// Detach RemoteMono (optional, destructor would do it automatically)
	mono.detach();

	return 0;
}
