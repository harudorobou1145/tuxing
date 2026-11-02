param(
    [string]$Source = "$PSScriptRoot/../../model/tiananmen/Tiananmen+Square+3D+Scene2.obj",
    [string]$Destination = "$PSScriptRoot/../../model/tiananmen/tiananmen_core.obj"
)
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Collections.Generic;
using System.Globalization;
public static class CoreSceneExtractor {
    sealed class Part {
        public string Name;
        public double MinX=double.PositiveInfinity, MinZ=double.PositiveInfinity;
        public double MaxX=double.NegativeInfinity, MaxZ=double.NegativeInfinity;
        public int Faces;
        public bool Keep;
    }
    static string[] Fields(string text) {
        return text.Split((char[])null, StringSplitOptions.RemoveEmptyEntries);
    }
    static int Index(string text, int count) {
        int value=int.Parse(text, CultureInfo.InvariantCulture);
        int index=value>0 ? value-1 : count+value;
        if(value==0 || index<0 || index>=count) throw new InvalidDataException("Invalid OBJ index: "+text);
        return index;
    }
    public static string Extract(string source, string destination) {
        var positions=new List<string>();
        var normals=new List<string>();
        var uvs=new List<string>();
        var points=new List<double[]>();
        var parts=new List<Part>();
        var part=new Part {Name="unnamed"};
        parts.Add(part);
        foreach(string raw in File.ReadLines(source)) {
            string line=raw.Trim();
            if(line.StartsWith("v ")) {
                var fields=Fields(line);
                positions.Add(line);
                points.Add(new [] {double.Parse(fields[1], CultureInfo.InvariantCulture), double.Parse(fields[3], CultureInfo.InvariantCulture)});
            } else if(line.StartsWith("vn ")) normals.Add(line);
            else if(line.StartsWith("vt ")) uvs.Add(line);
            else if(line.StartsWith("o ") || line.StartsWith("g ")) {
                part=new Part {Name=line.Substring(2)};
                parts.Add(part);
            } else if(line.StartsWith("f ")) {
                part.Faces++;
                var fields=Fields(line);
                for(int corner=1; corner<fields.Length; corner++) {
                    var point=points[Index(fields[corner].Split('/')[0], positions.Count)];
                    part.MinX=Math.Min(part.MinX, point[0]); part.MaxX=Math.Max(part.MaxX, point[0]);
                    part.MinZ=Math.Min(part.MinZ, point[1]); part.MaxZ=Math.Max(part.MaxZ, point[1]);
                }
            }
        }
        int kept=0;
        foreach(var candidate in parts) {
            candidate.Keep=candidate.Faces>0 && candidate.MinX>=-16000 && candidate.MaxX<=14000
                && candidate.MinZ>=-6000 && candidate.MaxZ<=26000;
            if(candidate.Keep) kept++;
        }
        if(kept==0) throw new InvalidDataException("No objects selected");
        var sources=new [] {positions, uvs, normals};
        var maps=new [] {new Dictionary<int,int>(),new Dictionary<int,int>(),new Dictionary<int,int>()};
        int[] seen=new int[3];
        int partIndex=0, faces=0;
        part=parts[0];
        using(var writer=new StreamWriter(destination, false, new System.Text.UTF8Encoding(false))) {
            writer.WriteLine("# Core scene extracted by code/tools/extract-scene.ps1");
            foreach(string raw in File.ReadLines(source)) {
                string line=raw.Trim();
                if(line.StartsWith("v ")) seen[0]++;
                else if(line.StartsWith("vt ")) seen[1]++;
                else if(line.StartsWith("vn ")) seen[2]++;
                else if(line.StartsWith("o ") || line.StartsWith("g ")) {
                    part=parts[++partIndex];
                    if(part.Keep) writer.WriteLine("o "+part.Name);
                } else if(line.StartsWith("f ") && part.Keep) {
                    var fields=Fields(line);
                    var corners=new string[fields.Length-1];
                    for(int corner=1; corner<fields.Length; corner++) {
                        var indices=fields[corner].Split('/');
                        for(int channel=0; channel<indices.Length; channel++) {
                            if(indices[channel].Length==0) continue;
                            int original=Index(indices[channel], seen[channel]);
                            int mapped;
                            if(!maps[channel].TryGetValue(original, out mapped)) {
                                mapped=maps[channel].Count+1;
                                maps[channel].Add(original, mapped);
                                writer.WriteLine(sources[channel][original]);
                            }
                            indices[channel]=mapped.ToString(CultureInfo.InvariantCulture);
                        }
                        corners[corner-1]=string.Join("/", indices);
                    }
                    writer.WriteLine("f "+string.Join(" ", corners));
                    faces++;
                }
            }
        }
        return string.Format("Objects: {0}/{1}; faces: {2}; vertices: {3}; bytes: {4}", kept, parts.Count, faces, maps[0].Count, new FileInfo(destination).Length);
    }
}
'@
[CoreSceneExtractor]::Extract([IO.Path]::GetFullPath($Source), [IO.Path]::GetFullPath($Destination))