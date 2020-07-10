using System;
using System.IO;
using static topologicsharp;
public class Test{
	public static void Main(String[] args){

	//	topologicsharp top = new topologicsharp();
		if(topologicsharp.init_stack() == null){
			Console.WriteLine("init_stack error");
				return;
		}

		//topologicsharp.graph g = new topologicsharp.graph(-1, (uint)START_STOP, 100, EDGES | FUNCTIONS, SWITCH, CONTINUE);
		Console.WriteLine("SUCCESS");

		
	}
}
