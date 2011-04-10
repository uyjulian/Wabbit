﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace WabbitC.Model.Statements
{
    class ReturnMove : ValueStatement
    {
        public Datum RValue;
        public ReturnMove(Datum src)
        {
            RValue = src;
        }

        public override ISet<Declaration> GetModifiedDeclarations()
        {
            return new HashSet<Declaration>() { Block.FindDeclaration("__hl") };
        }

        public override ISet<Declaration> GetReferencedDeclarations()
        {
            var temp = new HashSet<Declaration>();
			if (RValue.GetType() == typeof(Declaration))
				temp.Add((Declaration)RValue);
			return temp;
        }

        public override string ToString()
        {
			if (RValue.ToString() == "__hl")
				return "";
            return "__hl = " + RValue.ToString() + ";";
        }

		public override string ToAssemblyString()
		{
			if (RValue.ToString() == "__hl")
				return "";
			return "ld hl," + RValue.ToString();
		}
    }
}
