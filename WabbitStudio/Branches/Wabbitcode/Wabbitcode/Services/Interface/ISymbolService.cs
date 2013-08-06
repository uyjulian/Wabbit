﻿using Revsoft.Wabbitcode.Services.Symbols;

namespace Revsoft.Wabbitcode.Services.Interface
{
	public interface ISymbolService : IService
	{
		string ProjectDirectory { get; set; }

		SymbolTable SymbolTable { get; }
		ListTable ListTable { get; }
		void ParseSymbolFile(string symbolContents);
		void ParseListFile(string labelContents);
	}
}