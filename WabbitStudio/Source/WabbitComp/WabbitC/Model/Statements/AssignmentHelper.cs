﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using WabbitC.Model.Types;

namespace WabbitC.Model.Statements
{
    static class AssignmentHelper
    {
        public static ValueStatement ParseSingle(Block block, Declaration LValue, Token RValue)
        {
            if (Immediate.IsImmediate(RValue) == false)
            {
                Declaration valDecl = block.FindDeclaration(RValue.Text);
                return new Move(LValue, valDecl);
            }
            else
            {
                Immediate valImm = new Immediate(RValue);
                return new Assignment(LValue, valImm);
            }
        }

        public static void Parse(Block block, Declaration LValue, List<Token> tokenList)
        {
            Expression expr = new Expression(tokenList);

            var rpnStack = new Stack<Token>();
			var rpnList = expr.Eval();
			for (int i = rpnList.Count - 1; i >= 0; i--)
			{
				if (rpnList[i].Args != null)
				{
					//handle functions here
					Declaration funcDecl = block.FindDeclaration(rpnList[i].Tokens[0].Text);
					List<Declaration> paramList = FunctionCall.BuildParams(block, (FunctionType)funcDecl.Type, rpnList[i].Args);

					Declaration returnDecl;
					Type returnType = (funcDecl.Type as FunctionType).ReturnType;
					if (returnType.Equals(new BuiltInType("void")))
					{
						throw new Exception("Invalid type");
					}
					else
					{
						returnDecl = block.CreateTempDeclaration(returnType);
					}
					var funcCall = new FunctionCall(returnDecl, funcDecl, paramList);
					block.Statements.Add(funcCall);
					rpnStack.Push(Tokenizer.ToToken(returnDecl.Name));
				}
				else if (rpnList[i].Tokens[0].Type == TokenType.OperatorType)
				{
					var arg1 = rpnStack.Pop();
					Declaration decl = block.CreateTempDeclaration(TypeHelper.GetType(block, arg1));

					ValueStatement initialAssign = ParseSingle(block, decl, arg1);
					block.Statements.Add(initialAssign);

					var arg2 = rpnStack.Pop();
					if (Immediate.IsImmediate(arg2))
					{
						var operation = new AddImmediate(decl, new Immediate(arg2));
						block.Statements.Add(operation);
					}
					else
					{
						//TODO: check and make sure this is the same type/compatible as decl
						block.Statements.Add(new Add(decl, block.FindDeclaration(arg2.Text)));
					}

					rpnStack.Push(Tokenizer.ToToken(decl.Name));
				}
				else
				{
					rpnStack.Push(rpnList[i].Tokens[0]);
				}
			}

            var finalAssign = ParseSingle(block, LValue, rpnStack.Pop());
            block.Statements.Add(finalAssign);
        }
    }
}