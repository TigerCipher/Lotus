using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace LotusEditor.Utility
{
    public static class Serializer
    {
        public static void ToFile<T>(T instance, string path)
        {
            try
            {
                using var fs = new FileStream(path, FileMode.Create);
                var serializer = new DataContractSerializer(typeof(T));
                var settings = new XmlWriterSettings() { Indent = true };

                using var w = XmlWriter.Create(fs, settings);
                serializer.WriteObject(w, instance);
            }
            catch (Exception ex)
            {
                Logger.Error($"Failed to serialize {instance} to {path}");
                Debug.WriteLine(ex.Message);
                throw;
            }
        }

        public static T FromFile<T>(string path)
        {
            try
            {
                using var fs = new FileStream(path, FileMode.Open);
                var serializer = new DataContractSerializer(typeof(T));

                T instance = (T)serializer.ReadObject(fs)!;

                return instance;
            }
            catch (Exception ex)
            {
                Logger.Error($"Failed to deserialize {path}");
                Debug.WriteLine(ex.Message);
                throw;
            }
        }
    }
}
