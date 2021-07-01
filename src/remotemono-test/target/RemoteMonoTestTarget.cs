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

using System;
using System.Threading;
using System.Runtime.InteropServices;


public struct MyPoint
{
	public float x;
	public float y;
	
	public MyPoint(float x, float y)
	{
		this.x = x;
		this.y = y;
	}
	
	public override string ToString()
	{
		return "(" + x + ", " + y + ")";
	}
	
	public float length()
	{
		return (float) Math.Sqrt(x*x + y*y);
	}
}



public class RemoteMonoTestTarget
{

	static int TestValue = 42;
	static MyPoint PointValue = new MyPoint(1.0f, 2.0f);
	
	/*static int[] IntArrValue = new int[] {
		1, 2, 4, 8, 16, 32
	};*/
	static int[,] IntArrValue = new int[,] {
		{1, 2},
		{3, 4},
		{5, 6},
		{7, 8}
	};
	static string[] StrArrValue = new string[] {
		"Was",
		"yea",
		"ra",
		"chs",
		"hymmnos",
		"mea"
	};
	
	static string StringValue = "Hello World my dear.";
	
	static public int TestValueProp
	{
		get { return TestValue; }
		set { TestValueProp = value; }
	}
	
	public static void TestMain()
	{
		Console.WriteLine("Hello World from TestMain()!");
		
		while (true) {
			Thread.Sleep(1000);
		}
	}
	
	public static void HackMe(string hacker, ref int num, ref string didntHackWho, ref MyPoint whereIsHe)
	{
		Console.WriteLine("You've (" + didntHackWho + ") been hacked by " + hacker + " at " + whereIsHe + " " + num + " times!");
		num = 1337;
		didntHackWho = "Chuck Norris";
		whereIsHe.x *= 2.0f;
	}
	
	public static int DoAdd(int a, int b)
	{
		return a+b;
	}
	
	public static MyPoint GiveMeAPoint()
	{
		return new MyPoint(13.37f, 42.69f);
	}
	
	public static void DescribeArray(Array arr)
	{
		Console.WriteLine("Length: " + arr.Length);
		Console.WriteLine("Rank: " + arr.Rank);
		
		for (int dim = 0 ; dim < arr.Rank ; dim++) {
			Console.WriteLine("Dimension " + dim + ": " + (arr.GetUpperBound(dim) + 1));
		}
	}
	
	
	public void MethodNameThatShouldBeAsUniqueAsPossible1337420(string fubar, int blazeIt = 420)
	{
		Console.WriteLine(fubar + " - " + blazeIt);
	}
	
}


class RemoteMonoBase
{
	protected string protectedField;
	
	public virtual int Calculate(int a, int b)
	{
		return a+b;
	}
}


class RemoteMonoDerived : RemoteMonoBase
{
	public static int publicField;
	private string privateField;
	
	public static int PublicFieldProp
	{
		get { return publicField; }
		set { publicField = value; }
	}
	
	public string PrivateFieldProp
	{
		get { return privateField; }
		set { privateField = value; }
	}
	
	protected void ProtectedMethod(int a) {}
	int UnqualifiedMethod(int a, int b) { return a+b; }
	
	interface Nested
	{
		void DoSomething();
	}
	
	public override int Calculate(int a, int b)
	{
		return a*b;
	}
	
	public override string ToString()
	{
		return "I'm a RemoteMonoDerived instance";
	}
}


namespace remotemono
{

class RemoteMonoNamespacedClass
{
}

}


[StructLayout(LayoutKind.Explicit, Size=20)]
public class ClassWithExplicitLayout
{
	[FieldOffset(0)] public int IntAt0 = 123;
	[FieldOffset(10)] public int IntAt10 = 4567;
	[FieldOffset(15)] public int IntAt15 = 89;
}



public class FieldTest
{
	public int IntField = 13;
	public string StringField = "The quick brown fox";
	
	public static FieldTest Instance = new FieldTest();
	public static int StaticIntField = 25;
	public static string StaticStringField = "jumps over the lazy dog";
}


public struct ValFieldTest
{
	public string StringField;
	public int IntField;
	public MyPoint PointField;
	
	public static int StaticIntField = 64;
	public static ValFieldTest Instance = new ValFieldTest();
}


public class MethodTest
{
	public void SimpleMethod() { Console.WriteLine("MethodTest.SimpleMethod() called"); }
	public float AddFloat(float a, float b) { return a+b; }
	public float AddFloat(float a, float b, float c) { return a+b+c; }
	
	public string InterestingSignatureMethod(string op, int a, int b, ref float res)
	{
		if (op == "*") {
			res = a * b;
		} else if (op == "/") {
			res = a / (float) b;
		}
		return a + " " + op + " " + b + " = " + res;
	}
}


public class InvokeTest
{

	private string op;
	
	public static void DoAbsolutelyNothing() {}
	
	public static void DoAbsolutelyNothingWithOneArg(int a) {}
	
	public static int StaticAdd2(int a, int b) { return a+b; }
	
	public static MyPoint StaticGiveMeTwoPoints(float x1, float y1, float x2, float y2, out MyPoint p2)
	{
		p2 = new MyPoint(x2, y2);
		return new MyPoint(x1, y1);
	}
	
	public static MyPoint StaticPointMid(MyPoint a, MyPoint b) { return new MyPoint((a.x+b.x)*0.5f, (a.y+b.y)*0.5f); }
	
	public InvokeTest(string op)
	{
		this.op = op;
	}
	
	public int CalculateAndFormat(int a, int b, out string formatted)
	{
		int res = 0;
		if (op == "+") {
			res = a+b;
		} else if (op == "-") {
			res = a-b;
		}
		formatted = a + op + b + " = " + res;
		return res;
	}
	
	public int CalculateAndFormatWithPrefix(int a, int b, ref string formatted)
	{
		int res = 0;
		if (op == "+") {
			res = a+b;
		} else if (op == "-") {
			res = a-b;
		}
		string s = a + op + b + " = " + res;
		if (formatted != null) {
			formatted += s;
		} else {
			formatted = s;
		}
		return res;
	}
	
	public void ThrowIfNegative(float x)
	{
		if (x < 0.0f) {
			throw new ArgumentOutOfRangeException("x", "Parameter is negative!");
		}
	}
	
	public static string GiveMeAString()
	{
		return "AString";
	}
	
}


public class PropertyTest
{
	private string stringField;
	
	public float FloatProp { get; set; }
	
	public string StringProp
	{
		get { return stringField; }
		set { stringField = value; }
	}
	
	PropertyTest(float f, string s)
	{
		FloatProp = f;
		StringProp = s;
	}
}


public class NativeCallTest
{
	public static int StaticAdd3(int a, int b, int c)
	{
		return a+b+c;
	}
}






public class HelperFieldTest
{
	public int IntField = 13;
	public string StringField = "The quick brown fox";
	
	public static FieldTest Instance = new FieldTest();
	public static int StaticIntField = 25;
	public static string StaticStringField = "jumps over the lazy dog";
}


public struct HelperValFieldTest
{
	public string StringField;
	public int IntField;
	public MyPoint PointField;
	
	public static int StaticIntField = 64;
	public static ValFieldTest Instance = new ValFieldTest();
}


public class HelperNewObjectTest
{
	public int constructorUsed = 0;
	
	HelperNewObjectTest(float f, string s) { constructorUsed = 1; }
	HelperNewObjectTest(int i, string s) { constructorUsed = 2; }
}


public class HelperPropTest
{
	private static float staticFloatField = 13.37f;
	
	private string stringField;
	private int intField;
	
	public static float StaticFloatProp
	{
		get { return staticFloatField; }
		set { staticFloatField = value; }
	}
	
	public string StringProp
	{
		get { return stringField; }
		set { stringField = value; }
	}
	public int IntProp
	{
		get { return intField; }
		set { intField = value; }
	}
	
	HelperPropTest(string s, int i)
	{
		StringProp = s;
		IntProp = i;
	}
}


public class HelperMethodRetTypeTest
{
	public static string GiveMeAString() { return "AString"; }
}


public class BenchmarkTest
{
	public static MyPoint BuildMyPointWithPointlessStringArg(string arg, float x, float y) {
		return new MyPoint(x, y);
	}
}


public class GCFreeTestCounter
{
	public static int refcount = 0;
}

public class GCFreeTestObj
{
	public GCFreeTestObj()
	{
		GCFreeTestCounter.refcount++;
	}
	
	~GCFreeTestObj()
	{
		GCFreeTestCounter.refcount--;
	}
}


public enum SomeSimpleEnum1
{
	Never,
	Gonna,
	Give,
	You,
	Up
}

public enum SomeSimpleEnum2
{
	Never = -31,
	Gonna = 415,
	Let = 9,
	You = 2653,
	Down = -5
}

public enum SimpleByteEnum : byte
{
	Never = 1,
	Gonna = 2,
	Run = 3,
	Around = 5,
	And = 8,
	Desert = 13,
	You = 21
}
