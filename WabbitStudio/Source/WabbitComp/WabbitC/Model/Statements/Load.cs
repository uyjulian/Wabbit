﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace WabbitC.Model.Statements
{
    class Load : ValueStatement
    {
        public Declaration LValue;
        public Datum LoadAddress;

        public Load(Declaration LValue, Datum loadAddr)
        {
            this.LValue = LValue;
            this.LoadAddress = loadAddr;
        }


        public override List<Declaration> GetModifiedDeclarations()
        {
            return new List<Declaration>() { LValue };
        }

        public override List<Declaration> GetReferencedDeclarations()
        {
            var Result = new List<Declaration>();
            if (LoadAddress.GetType() == typeof(Declaration))
            {
                Result.Add(LoadAddress as Declaration);
            }
            return Result;
        }

        public override string ToString()
        {
            return this.LValue.Name + " = *" + this.LoadAddress.ToString() + ";";
        }
    }
}