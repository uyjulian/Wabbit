﻿using System;
using System.Collections.Generic;
using System.Text;

namespace WabbitC
{
    public class Expression
    {
        List<Token> tokens;
		public List<Token> Tokens
		{
			get { return tokens; }
		}

		List<Expression> args;
		public List<Expression> Args
		{
			get { return args; }
		}

        public Expression(List<Token> tokens)
        {
            this.tokens = tokens;
        }

		public Expression(Token token)
		{
			this.tokens = new List<Token> { token };
		}

        public List<Expression> Eval()
        {
			ReplaceDefines();
			var test = FillStack(this);
			CalculateStack(ref test);

			return test;
        }

		private void CalculateStack(ref List<Expression> expressions)
		{
			for (int i = expressions.Count - 1; i > 0; i--)
			{
				var exp = expressions[i];
				CalculateStack(ref exp);
				expressions[i] = exp;
			}
			var listTokens = Expression.ToTokens(expressions);
			var result = CalculateStack(ref listTokens);
			if (result != null)
			{
				expressions.Clear();
				expressions.Add(result);
			}
		}

		private void CalculateStack(ref Expression exp)
		{
			if (exp.Args != null)
				return;
			var evalExp = exp.Eval();
			var tokens = Expression.ToTokens(evalExp);
			var result = CalculateStack(ref tokens);
			exp = result == null ? exp : result;
		}

		private Expression CalculateStack(ref List<Token> tokens)
		{
			if (tokens.Count < 2)
				return null;
			if (tokens.Count == 2)
			{
				Token op = tokens[0];
				Token value = tokens[1];
				var result = ApplyOperator(value, op);
				return result;
			}
			else if (tokens.Count == 3 )
			{
				Token op = tokens[0];
				if (op.Type == TokenType.OperatorType)
				{
					Token value1 = tokens[1];
					Token value2 = tokens[2];
					Expression result = ApplyOperator(value1, value2, op);
					return result;
				}
			}
			return null;
		}

		private Expression ApplyOperator(WabbitC.Token tok1, WabbitC.Token op)
		{
			Expression result = null;
			switch (op.Text)
			{
				case "!":
					result = !tok1;
					break;
				case "~":
					result = ~tok1;
					break;
			}
			return result;
		}

		private Expression ApplyOperator(WabbitC.Token tok1, WabbitC.Token tok2, WabbitC.Token op)
		{
			int int1, int2;
			Token token;
			Expression result = null;
			switch (op.Text)
			{
				case "+":
					result = tok1 + tok2;
					break;
				case "−":
				case "-":
					result = tok1 - tok2;
					break;
				case "*":
					result = tok1 * tok2;
					break;
				case "/":
					result = tok1 / tok2;
					break;
				case "^":
					result = tok1 ^ tok2;
					break;
                case "=":
                    result = (Token.OpEquals(tok1, tok2));
                    break;
				case "&":
					result = tok1 & tok2;
					break;
				case "|":
					result = tok1 | tok2;
					break;
				case "||":
					if (!int.TryParse(tok1.Text, out int1) || !int.TryParse(tok2.Text, out int2))
					{
						var resultList = new List<Token> { Token.OROperatorToken, tok2, tok1 };
						return new Expression(resultList);
					}
					token = new Token() { Text = (Convert.ToBoolean(int1) || Convert.ToBoolean(int2)).ToString(), Type = TokenType.IntType };
					return new Expression(token);
				case "&&":
					if (!int.TryParse(tok1.Text, out int1) || !int.TryParse(tok2.Text, out int2))
					{
						var resultList = new List<Token> { Token.OROperatorToken, tok2, tok1 };
						return new Expression(resultList);
					}
					token = new Token() { Text = (Convert.ToBoolean(int1) && Convert.ToBoolean(int2)).ToString(), Type = TokenType.IntType };
					return new Expression(token);
			}
			return result;
		}

		public List<Expression> FillStack(Expression input)
		{
			List<Expression> stack = new List<Expression>();
			stack.Add(input);
			for (int i = 0; i < stack.Count; i++)
			{
				Expression curExpr = stack[i];
				int operatorIndex = Expression.GetOperator(curExpr);
				if (operatorIndex != -1)
				{
					int j;
					Expression leftSide, rightSide, op;
					List<Token> tokenList = new List<Token>();
					for (j = 0; j < operatorIndex; j++)
						tokenList.Add(curExpr.Tokens[j]);
					leftSide = new Expression(tokenList);
					tokenList = new List<Token>();
					op = new Expression(new List<Token> { curExpr.Tokens[j++] });
					for (; j < curExpr.Tokens.Count; j++)
						tokenList.Add(curExpr.Tokens[j]);
					rightSide = new Expression(tokenList);
					stack.Remove(curExpr);
					stack.Insert(i, op);
					stack.Insert(i + 1, leftSide);
					stack.Insert(i + 2, rightSide);
				}
				else
				{
					if (curExpr.Tokens.Count < 1)
						continue;
					Token curToken = curExpr.Tokens[0];
					if (curToken.Type == TokenType.OpenParen)
					{
						//handle paren
						int nParen = 0;
						int j = 1;
						curToken = curExpr.Tokens[j];
						List<Token> insideTokens = new List<Token>();
						while (!(curToken.Type == TokenType.CloseParen && nParen == 0))
						{
							if (curToken.Type == TokenType.CloseParen)
								nParen--;
							else if (curToken.Type == TokenType.OpenParen)
								nParen++;
							insideTokens.Add(curToken);
							curToken = curExpr.Tokens[++j];
						}
						Expression insideExp = new Expression(insideTokens);
						stack[i] = insideExp;
					}
					else if (curToken.Type == TokenType.StringType && curExpr.Tokens.Count > 1 &&
								curExpr.Tokens[1].Type == TokenType.OpenParen)
					{
						//its a func!
						List<Expression> args = new List<Expression>();
						curToken = curExpr.Tokens[2];
						while (curToken.Type != TokenType.CloseParen)
						{
							int nParen = 0;
							List<Token> arg = new List<Token>();
							while (!((curToken.Type == TokenType.ArgSeparator || curToken.Type == TokenType.CloseParen) && nParen == 0))
							{
								if (curToken.Type == TokenType.CloseParen)
									nParen++;
								else if (curToken.Type == TokenType.OpenParen)
									nParen--;
								arg.Add(curToken);
								curExpr.Tokens.Remove(curToken);
								curToken = curExpr.Tokens[2];
							}
							Expression argExp = new Expression(arg);
							args.Add(argExp);
							if (3 < curExpr.Tokens.Count)
							{
								curExpr.Tokens.Remove(curToken);
								curToken = curExpr.Tokens[2];
							}
						}
						curExpr.args = args;
					}
				}
			}
			return stack;
		}

		static List<List<string>> operators = new List<List<string>> { 
																	new List<string> {"="},
																	new List<string> {"||"},
																	new List<string> {"&&"}, 
																	new List<string> {"|"},
																	new List<string> {"^"},
																	new List<string> {"&"},
																	new List<string> {"==", "!="},
																	new List<string> {"<", "<=", ">", ">="},
																	new List<string> {">>", "<<"},
																	new List<string> {"+", "-", "−"},
																	new List<string> {"*", "/", "%"},
																	new List<string> {"!", "~"},
																	new List<string> {"++",  "−−", "--"},
																};
		public static int GetOperator(Expression expr)
		{
			for (int level = 0; level < operators.Count; level++)
			{
				List<string> operatorLevel = operators[level];
				List<Token> tokens = expr.Tokens;
				int nParen = 0;
				bool leftToRight =  GetOpAssoc(level);
				for (int i = leftToRight ? tokens.Count - 1 : 0;  leftToRight ? i >= 0 : i < tokens.Count; i += leftToRight ? -1 : 1)
				{
					Token token = tokens[i];
					if (token.Type == TokenType.CloseParen)
						nParen++;
					else if (token.Type == TokenType.OpenParen)
						nParen--;
					else if (nParen == 0 && token.Type == TokenType.OperatorType && operatorLevel.Contains(token.Text)) 
					{
						if ((token.Text != "-" && token.Text != "+" && token.Text != "*") 
								|| (i > 0 && tokens[i - 1].Type != TokenType.OperatorType))
							return i;
					}
				}
			}
			return -1;
		}


		/// <summary>
		/// Checks if this operator goes right to left or left right
		/// </summary>
		/// <param name="level">level of operators were on</param>
		/// <returns>True if the Association is left to right. False otherwise</returns>
		private static bool GetOpAssoc(int level)
		{
			switch (level)
			{
				case 0:
				case 11:
					return false;
				default:
					return true;
			}
		}

		#region Defines
		private void ReplaceDefines()
		{
			for(int i = 0; i < tokens.Count; i++)
			{
				Token token = tokens[i];
				CheckTokenReplace(ref token);
				tokens[i] = token;
			}
		}

		private void CheckTokenReplace(ref WabbitC.Token token)
		{
			if (token.Type == TokenType.StringType)
			{
				PreprocessorDefine define = PreprocessorParser.DefineValue(token);
				if (define != null)
				{
					Type defineType = define.GetType();
					if (defineType == typeof(ReplacementDefine))
					{
						ReplacementDefine replaceDefine = (ReplacementDefine)define;
						token = replaceDefine.Value;
						CheckTokenReplace(ref token);
					}
				}
			}
		}
		#endregion

		public static List<Token> ToTokens(List<Expression> exps)
		{
			var evalTokens = new List<Token>();
			foreach(Expression exp in exps)
				evalTokens.AddRange(exp.Tokens);
			return evalTokens;
		}

		public override string ToString()
		{
			StringBuilder sb = new StringBuilder();
			for(int i = 0; i < tokens.Count - 1; i++)
			{
				sb.Append(tokens[i].Text);
			}
			if (args != null)
			{
				foreach (Expression arg in args)
				{
					sb.Append(arg.ToString());
					sb.Append(",");
				}
				sb.Remove(sb.Length - 1, 1);
			}
			sb.Append(tokens[tokens.Count - 1]);
			return sb.ToString();
		}
    }

	public class ExpressionResult
	{
		public ExpressionResult()
		{

		}
	}
}
