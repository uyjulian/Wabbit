﻿Public Class XObjectLayer
    Inherits ZBaseLayer(Of ZObject)

    Public Overrides ReadOnly Property LayerType As LayerType
        Get
            Return WPFZ80MapEditor.LayerType.ObjectLayer
        End Get
    End Property

    Protected Overrides Function FinishDrop(Def As ZDef, Args As IList(Of Object))
        Return ZObject.FromDef(Def, Args)
    End Function

End Class
