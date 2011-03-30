﻿using WabbitC.TokenPasses;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using WabbitC;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace WabbitC_Tests
{
    
    
    /// <summary>
    ///This is a test class for BracerTest and is intended
    ///to contain all BracerTest Unit Tests
    ///</summary>
	[TestClass()]
	public class BracerTest
	{


		private TestContext testContextInstance;

		/// <summary>
		///Gets or sets the test context which provides
		///information about and functionality for the current test run.
		///</summary>
		public TestContext TestContext
		{
			get
			{
				return testContextInstance;
			}
			set
			{
				testContextInstance = value;
			}
		}

		#region Additional test attributes
		// 
		//You can use the following additional attributes as you write your tests:
		//
		//Use ClassInitialize to run code before running the first test in the class
		//[ClassInitialize()]
		//public static void MyClassInitialize(TestContext testContext)
		//{
		//}
		//
		//Use ClassCleanup to run code after all tests in a class have run
		//[ClassCleanup()]
		//public static void MyClassCleanup()
		//{
		//}
		//
		//Use TestInitialize to run code before running each test
		//[TestInitialize()]
		//public void MyTestInitialize()
		//{
		//}
		//
		//Use TestCleanup to run code after each test has run
		//[TestCleanup()]
		//public void MyTestCleanup()
		//{
		//}
		//
		#endregion


		/// <summary>
		///A test for Run
		///</summary>
		[TestMethod()]
		public void RunTest1()
		{
			string fileContents = Compiler.TryOpenFile(@"..\..\..\WabbitC Tests\bracer_test.c");
			Assert.IsTrue(!string.IsNullOrEmpty(fileContents));
			List<Token> tokenList = Tokenizer.Tokenize(fileContents);

			fileContents = Compiler.TryOpenFile(@"..\..\..\WabbitC Tests\bracer_result.c");
			Assert.IsTrue(!string.IsNullOrEmpty(fileContents));
			List<Token> expected = Tokenizer.Tokenize(fileContents);
			
			Bracer target = new Bracer();
			List<Token> actual = target.Run(tokenList);

			Assert.IsTrue(expected.SequenceEqual(actual));
		}

        [TestMethod()]
        public void RunTest2()
        {
            Bracer target = new Bracer(); // TODO: Initialize to an appropriate value

            List<Token> tokenList = Tokenizer.Tokenize("do if (test) test = 0; while (1); var = 2;");
            List<Token> expected = Tokenizer.Tokenize("do {if (test) {test = 0;}} while (1); var = 2;");

            List<Token> actual;
            actual = target.Run(tokenList);

            Assert.IsTrue(expected.SequenceEqual<Token>(actual),
                "Expected: \"" + string.Join<Token>(" ", expected.ToArray()) + "\" " +
                "Actual: \"" + string.Join<Token>(" ", actual.ToArray()) + "\"");
        }

        [TestMethod()]
        public void RunTest3()
        {
            Bracer target = new Bracer(); // TODO: Initialize to an appropriate value

            List<Token> tokenList = Tokenizer.Tokenize("do { while (1) i += 1; } while (1);");
            List<Token> expected = Tokenizer.Tokenize("do { while (1) {i += 1;} } while (1);");

            List<Token> actual;
            actual = target.Run(tokenList);

            Assert.IsTrue(expected.SequenceEqual<Token>(actual),
                "Expected: \"" + string.Join<Token>(" ", expected.ToArray()) + "\" " +
                "Actual: \"" + string.Join<Token>(" ", actual.ToArray()) + "\"");
        }

        [TestMethod()]
        public void RunTest4()
        {
            Bracer target = new Bracer(); // TODO: Initialize to an appropriate value

            List<Token> tokenList = Tokenizer.Tokenize("do { do if (test) while (0) test = 0; while (1); } while (1);");
            List<Token> expected = Tokenizer.Tokenize("do { do {if (test) {while (0) {test = 0;}}} while (1); } while (1);");

            List<Token> actual;
            actual = target.Run(tokenList);

            Assert.IsTrue(expected.SequenceEqual<Token>(actual),
                "Expected: \"" + string.Join<Token>(" ", expected.ToArray()) + "\" " +
                "Actual: \"" + string.Join<Token>(" ", actual.ToArray()) + "\"");
        }
	}
}
