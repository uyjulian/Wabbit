﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using WabbitC.Model;
using WabbitC.Model.Statements;
using WabbitC.Model.Statements.Math;

namespace WabbitC.Optimizer
{
    static class DeadCodeOptimizer
    {
        static List<OptimizerSymbol> symbolTable = new List<OptimizerSymbol>();
        public static void Optimize(ref Module module)
        {
            for (int i = 0; i < module.Declarations.Count; i++)
            {
                Declaration decl = module.Declarations[i];
				if (decl.Code != null)
					OptimizeBlock(ref decl.Code);
            }
        }

        public static void OptimizeBlock(ref Block block)
        {
			//true if live, dead if false
            bool[] instructions = new bool[block.Statements.Count];
            bool changedInstructions;
            do
            {
				for (int i = 0; i < instructions.Length; i++)
				{
					instructions[i] = true;
				}
                changedInstructions = false;
                for (int i = 0; i < block.Statements.Count; i++)
                {
                    var statement = block.Statements[i];
                    var type = statement.GetType();
                    if (type == typeof(Assignment))
					{
						var assignment = statement as Assignment;
						var decl = block.FindDeclaration(assignment.LValue.Name);
						var isReferenced = IsReferenced(block, instructions, assignment.LValue, i + 1);
						if (decl != null && !isReferenced)
						{
							instructions[i] = false;
							changedInstructions = true;
						}
					}
					else if (type.BaseType == typeof(MathStatement))
					{
						var math = statement as MathStatement;
						var decl = block.FindDeclaration(math.LValue.Name);
						var isReferenced = IsReferenced(block, instructions, math.LValue, i + 1);
						if (decl != null && !isReferenced)
						{
							instructions[i] = false;
							changedInstructions = true;
						}
					}
					else if (type.BaseType == typeof(ConditionStatement))
					{
						var cond = statement as ConditionStatement;
						var decl = block.FindDeclaration(cond.LValue.Name);
						var isReferenced = IsReferenced(block, instructions, cond.LValue, i + 1);
						if (decl != null && !isReferenced)
						{
							instructions[i] = false;
							changedInstructions = true;
						}
					}
					else if (type == typeof(Move))
					{
						var move = statement as Move;
						var decl = block.FindDeclaration(move.LValue.Name);
						var isReferenced = IsReferenced(block, instructions, move.LValue, i + 1);
						if (decl != null && !isReferenced)
						{
							instructions[i] = false;
							changedInstructions = true;
						}
					}
					else if(type == typeof(FunctionCall))
					{
						var func = statement as FunctionCall;
						var decl = block.FindDeclaration(func.LValue.Name);
						var isReferenced = IsReferenced(block, instructions, func.LValue, i + 1);
						if (decl != null && !isReferenced)
						{
							instructions[i] = false;
							changedInstructions = true;
						}
					}
					else
					{

					}
                }
                if (changedInstructions)
                {
                    var newStatements = new BlockStatements(block);
                    for (int i = 0; i < block.Statements.Count; i++)
                    {
                        if (instructions[i])
                            newStatements.Add(block.Statements[i]);
                    }
                    block.Statements = newStatements;
                }
            } while (changedInstructions);
        }

        private static bool IsReferenced(Block block, bool[] deadInstructions, Declaration declaration, int i)
        {
            int isRefed = -1;
			int isReassigned = -1;
            for (; i < block.Statements.Count && isRefed == -1; i++)
            {
                var statement = block.Statements[i];
				var modified = statement.GetModifiedDeclarations();
				var refed = statement.GetReferencedDeclarations();
				var type = statement.GetType();
				if ((type == typeof(Move) || type == typeof(Assignment)) && modified.Contains(declaration))
					isReassigned = i;
				if (refed.Contains(declaration) && deadInstructions[i])
					isRefed = i;
            }
			if (isReassigned < isRefed && isReassigned >= 0 && isRefed >= 0)
				return false;
            return isRefed >= 0;
        }
    }
}