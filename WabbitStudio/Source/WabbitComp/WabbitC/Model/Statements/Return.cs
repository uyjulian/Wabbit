﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace WabbitC.Model.Statements
{
    class Return : ControlStatement
    {
        public Declaration ReturnReg;

        public Return(Declaration decl)
        {
            ReturnReg = decl;
        }

        public override List<Declaration> GetReferencedDeclarations()
        {
            if (ReturnReg != null && ReturnReg.GetType() == typeof(Declaration))
            {
                return new List<Declaration>() { ReturnReg as Declaration };
            }
            else
            {
                return base.GetReferencedDeclarations();
            }
        }

        public override string ToString()
        {
			if (ReturnReg == null)
				return "return;";
            return "return " + ReturnReg + ";";
        }
    }
}
