﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;

using WabbitC.Model.Types;
using System.Collections;

namespace WabbitC.Model
{
    class Module : Block
    {
		public List<string> IntermediateStrings;
		public ISet<Declaration> GeneralPurposeRegisters;
		public ISet<Declaration> Registers;
        public List<KeyValuePair<Declaration, List<int>>> GlobalVariables = new List<KeyValuePair<Declaration, List<int>>>();

		public Module() { }

        public void UpdateModule(Block block)
        {
            this.Types = block.Types;
            this.Declarations = block.Declarations;
			IntermediateStrings = new List<string>();

			var gprs = new HashSet<Declaration>()
            {
                new Declaration(new BuiltInType("int"), "__hl"),
                new Declaration(new BuiltInType("int"), "__de"),
                new Declaration(new BuiltInType("int"), "__bc"),
				new Declaration(new BuiltInType("char"), "__h"),
				new Declaration(new BuiltInType("char"), "__l"),
				new Declaration(new BuiltInType("char"), "__d"),
				new Declaration(new BuiltInType("char"), "__e"),
				new Declaration(new BuiltInType("char"), "__b"),
				new Declaration(new BuiltInType("char"), "__c"),
				new Declaration(new BuiltInType("char"), "__a"),
			};

			var regs = new HashSet<Declaration>()
			{
				new Declaration(new BuiltInType("int"), "__iy"),
				new Declaration(new BuiltInType("int"), "__ix"),
				new Declaration(new BuiltInType("int"), "__sp")
			};
			regs.UnionWith(gprs);

			GeneralPurposeRegisters = gprs;
			Registers = regs;

			Declarations.Insert(0, new Declaration(new Types.Array(new BuiltInType("unsigned char"), "[2048]"), "__stack"));
			Declarations.InsertRange(0, Registers);

			//TODO: make so assignment statements work...
			block.Parent = this;
            this.Parent = null;
        }

        int tempGlobals = 0;
        private string NewTempGlobal()
        {
            return "__global" + tempGlobals++;
        }

		public Declaration AllocateGlobalVariable(Declaration decl, int level)
		{
            foreach (var possibleOverlap in GlobalVariables) {
                if (possibleOverlap.Key.Type.Equals(decl.Type) && !possibleOverlap.Value.Contains(level)) {
                    possibleOverlap.Value.Add(level);
                    return possibleOverlap.Key;
                }
            }
            var newDecl = new Declaration(decl.Type, NewTempGlobal());
            GlobalVariables.Add(new KeyValuePair<Declaration, List<int>>(newDecl, new List<int>() { level }));
            return newDecl;
		}

        public IEnumerator<Declaration> GetFunctionEnumerator()
        {
            var functions = from d in Declarations
                            where (d.Type.GetType() == typeof(FunctionType) || d.Type.GetType().BaseType == typeof(FunctionType)) && d.Code != null
                            select d;
            return functions.GetEnumerator();
        }

        static public Module ParseModule(ref List<Token>.Enumerator tokens)
        {
			Module mod = new Module();
            var block = Block.ParseBlock(ref tokens, mod, null);
            // Make sure there's no stuff that doesn't belong in module
			mod.UpdateModule(block);
            return mod;
        }

		public override string ToString()
		{
			string result = "";
			foreach (string s in IntermediateStrings)
			{
				result += s + Environment.NewLine;
			}
            foreach (var variable in GlobalVariables)
            {
                result += variable.Key.ToDeclarationString() + Environment.NewLine;
            }
			result += base.ToString();
			return result;
		}
	}
}
