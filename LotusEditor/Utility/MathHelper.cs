using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LotusEditor.Utility
{
    internal static class MathHelper
    {
        public static float Epsilon => 0.00001f;

        public static bool IsEqual(this float value, float other)
        {
            return Math.Abs(value - other) < Epsilon;
        }

        public static bool IsEqual(this float? value, float? other)
        {
            if (!value.HasValue || !other.HasValue) return false;
            return Math.Abs(value.Value - other.Value) < Epsilon;
        }
    }
}
