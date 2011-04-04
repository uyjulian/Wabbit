﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using WabbitC.Model;
using WabbitC.Model.Statements;
using WabbitC.Model.Statements.Math;
using WabbitC.Model.Types;

namespace WabbitC.StatementPasses
{
	class ApplyCallingConvention
	{
		public static void Run(Module module)
		{
			var functions = module.GetFunctionEnumerator();
			while (functions.MoveNext())
			{
				Block block = functions.Current.Code;

				var statements = from Statement s in block
								 where s.GetType() == typeof(FunctionCall)
								 select s;
				foreach (FunctionCall call in statements)
				{
					int index = block.Statements.IndexOf(call);
					block.Statements.Remove(call);

					var newStatements = new List<Statement>();

					for (int i = 0; i < call.Params.Count; i++)
					{
						newStatements.Add(new Push(call.Params[i]));
					}

					// Garbage push representing return address
					if (!block.Function.ReturnType.Equals(BuiltInType.BuiltInTypeType.Void))
						newStatements.Add(new Push(block.FindDeclaration("__hl")));

					newStatements.Add(new Call(call.Function));
					if (call.LValue != null)
					{
						newStatements.Add(new Move(call.LValue, block.FindDeclaration("__hl")));
					}

					block.Statements.InsertRange(index, newStatements);


				}

				// Add popping regs
				Label endLabel = new Label("__function_cleanup");

				var returns = from Statement s in block
					where s.GetType() == typeof(Return)
					select s;
				foreach (Return curReturn in returns)
				{
					int index = block.Statements.IndexOf(curReturn);
					block.Statements.Remove(curReturn);

					var newStatements = new List<Statement>();

					var assignment = block.Statements[index - 1];
					if (assignment.GetType() == typeof(Assignment))
					{
						block.Statements.Remove(assignment);
						index--;
						curReturn.ReturnReg.ConstValue = ((Assignment)assignment).RValue;
					}
					else
					{
						curReturn.ReturnReg.ConstValue = null;
					}
					newStatements.Add(new ReturnMove(curReturn.ReturnReg));
					newStatements.Add(new Goto(endLabel));
					block.Statements.InsertRange(index, newStatements);
				}

				block.Statements.Add(endLabel);

				var funcType = functions.Current.Type as FunctionType;
				var newFuncType = new StrippedFunctionType(funcType);
				functions.Current.Type = newFuncType;
			}
		}
	}
}