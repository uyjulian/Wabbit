﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using WabbitC.Model;
using WabbitC.Model.Statements;
using WabbitC.Model.Statements.Math;
using System.Diagnostics;

namespace WabbitC.Optimizer
{
	static class CSE
	{
		public static bool Optimize(ref Module module)
		{
			bool hasChanged = false;
			var functions = module.GetFunctionEnumerator();
			while (functions.MoveNext())
			{
				Block block = functions.Current.Code;
				var basicBlocks = block.GetBasicBlocks();
				block.Statements.Clear();
				for (int i = 0; i < basicBlocks.Count; i++)
				{
					Block basicBlock = basicBlocks[i];
					hasChanged |= OptimizeBlock(ref basicBlock);
					block.Statements.AddRange(basicBlock);
				}
			}
			return hasChanged;
		}

		/*Algorithm:
		 * Source: http://www.cs.montana.edu/~bohannan/550/local_subexp_elim.html
		For each binary expression (x = z + 2, for example) in the basic block:
			If the expression, or its equivalent, has not yet been seen,
				Add the expression to our set of possible redundant expressions.
			If the expression, or its equivalent, has been seen exactly once before,
				1. Create a temporary variable to hold the computation.
				2. Add an instruction immediatly prior to the first time the instruction was seen.
				3. Replace the first and second occurances with the temporary variable.
			If the expression, or its equivalent, has been seen more than twice,
				Replace the expression with its temporary variable.
		*/
		public static bool OptimizeBlock(ref Block block)
		{
			List<CSEStore> possibleSubExp = new List<CSEStore>();
			CSEStore curExp = null;
			bool hasChanged = false;
			var statements = from Statement st in block select st;
			foreach (var statement in statements)
			{
				int index = block.Statements.IndexOf(statement);
				block.Statements.Remove(statement);
				var newStatements = new List<Statement>();

				//actual optimization code
				System.Type type = statement.GetType();
				if (type == typeof(Move))
				{
					var move = statement as Move;
					curExp = new CSEStore(index, move.LValue, move.RValue);
				}
				else if (type.BaseType == typeof(ConditionStatement))
				{
					var cond = statement as ConditionStatement;
					curExp = new CSEStore(index, cond.CondDecl, cond.CondValue, cond.Operator);
				}
				else if (type.BaseType == typeof(MathStatement))
				{
					Debug.Assert(curExp != null);
					var math = statement as MathStatement;
					curExp.Operand2 = math.RValue;
					curExp.Operator = math.Operator;
					int listIndex = possibleSubExp.IndexOf(curExp);
					if (listIndex == -1)
					{
						possibleSubExp.Add(curExp);
					}
					else
					{
						var firstInst = possibleSubExp[listIndex];
						if (firstInst.TempDecl == null)
						{
							firstInst.TempDecl = block.CreateTempDeclaration(firstInst.LValue.Type);
							block.Statements.Insert(firstInst.index + 2, new Move(firstInst.TempDecl, firstInst.LValue));
						}
						block.Statements.RemoveAt(index);						
						newStatements.Add(new Move(firstInst.LValue, firstInst.TempDecl));
						hasChanged = true;
					}
					curExp = new CSEStore(index, math.LValue, math.RValue);
				}

				if (newStatements.Count == 0)
					newStatements.Add(statement);
				block.Statements.InsertRange(index, newStatements);
			}
			return hasChanged;
		}

		class CSEStore
		{
			public Declaration TempDecl = null;
			public int index;
			public Datum Operand1;
			public Datum Operand2;
			public Token Operator;
			public Declaration LValue;

			public CSEStore(int index, Declaration lValue, Datum op1)
			{
				this.index = index;
				LValue = lValue;
				Operand1 = op1;
			}

			public CSEStore(int index, Datum op1, Datum op2, Token op)
			{
				this.index = index;
				Operand1 = op1;
				Operand2 = op2;
				Operator = op;
			}

			public override bool Equals(object obj)
			{
				if (obj.GetType() != typeof(CSEStore))
					return base.Equals(obj);
				var cse = obj as CSEStore;
				return cse.Operand1 == this.Operand1 && cse.Operand2 == this.Operand2 && cse.Operator == this.Operator;
			}

			public override string ToString()
			{
				string temp = Operand1.ToString();
				if (Operand2 != null)
				{
					temp += Operator + Operand2;
				}
				return temp;
			}
		}
	}
}
