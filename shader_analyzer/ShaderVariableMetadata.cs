using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace shader_analyzer
{
    //
    // Extracts the constant buffers and variables inside from the runtime dump
    //
    class ShaderVariableMetadata
    {
        public class CBufferVariable
        {
            public string Type;
            public string Name;
            public int Offset;
            public int Size;
        }

        public class CBuffer
        {
            public string Name;
            public int Register;
            public List<CBufferVariable> Variables;
        }

        private readonly string[] m_FileLines;
        private List<CBuffer> m_Buffers;

        public ShaderVariableMetadata(string FilePath)
        {
            // This will throw an exception if FilePath is invalid anyway - don't
            // bother checking it beforehand.
            m_FileLines = System.IO.File.ReadAllLines(FilePath);
            m_Buffers = new List<CBuffer>();

            //
            // Example format for each constant buffer:
            //
            // cbuffer PerMaterial : register(b1)
            // {
            //      float4 Color2   : packoffset(c0);   // @ 0 - 0x0000
            //      float4 Color3   : packoffset(c1);   // @ 4 - 0x0010
            //      float4 Velocity : packoffset(c2);   // @ 8 - 0x0020
            //      float4 fVars3[3]: packoffset(c10);  // @ 40 - 0x00A0
            // }
            //
            Regex bufferDeclExpr = new Regex(@"cbuffer\s+(.*?)\s*:\s+register\(b(.*?)\)", RegexOptions.Compiled);
            Regex varDeclExpr = new Regex(@"^\s*(.*?)\s+(.*?)\s*:\s+packoffset\(c(.*?)\);.*?", RegexOptions.Compiled);

            CBuffer currentBuffer = null;
            bool needsBracket = false;

            foreach (string line in m_FileLines)
            {
                if (string.IsNullOrWhiteSpace(line))
                    continue;

                if (currentBuffer == null)
                {
                    var bufferMatch = bufferDeclExpr.Match(line);

                    if (!bufferMatch.Success || bufferMatch.Groups.Count < 3)
                        continue;

                    currentBuffer = new CBuffer()
                    {
                        Name = bufferMatch.Groups[1].Value,
                        Register = int.Parse(bufferMatch.Groups[2].Value),
                        Variables = new List<CBufferVariable>()
                    };

                    needsBracket = true;
                    continue;
                }

                // After a buffer declaration, the next line must be a bracket
                if (needsBracket)
                {
                    if (line.Equals("{"))
                        needsBracket = false;
                    else
                        throw new FormatException("Unknown buffer declaration format");

                    continue;
                }

                // If it's a closing bracket, said buffer is finished
                if (line.Equals("}"))
                {
                    m_Buffers.Add(currentBuffer);
                    currentBuffer = null;

                    continue;
                }

                // The only other option is a variable declaration
                var variableMatch = varDeclExpr.Match(line.Replace("row_major", "").Trim());

                if (!variableMatch.Success || variableMatch.Groups.Count < 4)
                    throw new FormatException("Unknown variable declaration format");

                string type = variableMatch.Groups[1].Value;
                string name = variableMatch.Groups[2].Value;

                currentBuffer.Variables.Add(new CBufferVariable()
                {
                    Type = type,
                    Name = name,
                    Offset = ExtractVariableOffset(variableMatch.Groups[3].Value),
                    Size = ExtractVariableSize(type, name)
                });
            }
        }

        public string[] GetTextData()
        {
            return m_FileLines;
        }

        public List<CBuffer> GetBuffers()
        {
            return m_Buffers;
        }

        private int ExtractVariableOffset(string Input)
        {
            int modifier = 0;

            if (Input.Contains(".x"))
                modifier = 0;
            else if (Input.Contains(".y"))
                modifier = 4;
            else if (Input.Contains(".z"))
                modifier = 8;
            else if (Input.Contains(".w"))
                modifier = 12;

            Input = Input.Replace(".x", "").Replace(".y", "").Replace(".z", "").Replace(".w", "");

            // Multiply by float4 which is 16 bytes each
            return (int.Parse(Input) * 16) + modifier;
        }

        private int ExtractVariableSize(string Type, string Name)
        {
            Type = Type.Replace("row_major", "").Replace("col_major", "").Trim();
            int baseSize = 0;

            if (Type.Equals("float4x4"))
                baseSize = 4 * 4 * 4;// 4x4 * sizeof(float)
            else if (Type.Equals("float3x4"))
                baseSize = 4 * 3 * 4;// 4x3 * sizeof(float)
            else if (Type.Equals("float4"))
                baseSize = 4 * 4;// 4 * sizeof(float)
            else if (Type.Equals("float3"))
                baseSize = 4 * 3;// 3 * sizeof(float)
            else if (Type.Equals("float2"))
                baseSize = 4 * 2;// 2 * sizeof(float)
            else if (Type.Equals("float"))
                baseSize = 4 * 1;// 1 * sizeof(float)
            else if (Type.Equals("undefined") || Type.Equals("unknown"))
                baseSize = 1;
            else
                throw new FormatException($"Unhandled variable type '{Type}'");

            // Now check if the name was really an array
            int arrayOperatorIndex = Name.IndexOf('[');
            int arrayOperatorEnd = Name.IndexOf(']');
            int arrayModifier = 1;

            if (arrayOperatorIndex != -1)
                arrayModifier = int.Parse(Name.Substring(arrayOperatorIndex + 1, arrayOperatorEnd - arrayOperatorIndex - 1));

            return baseSize * arrayModifier;
        }
    }
}
