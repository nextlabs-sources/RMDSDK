using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace CustomControls.pages.CentralPolicy.model
{
    public class UserSelectTags
    {
        // List<tag<tagName,tagvalues> >
        private List<KeyValuePair<string, List<string>>> tags;

        public UserSelectTags()
        {
            this.tags = new List<KeyValuePair<string, List<string>>>();
        }

        public void AddTag(string tagName, string tagValue)
        {
            var node = tags.Find((i) =>
            {
                if (i.Key.Equals(tagName))
                {
                    return true;
                }
                else
                {
                    return false;
                }

            });

            if (node.Key != null && node.Key.Equals(tagName))
            {
                if (!node.Value.Contains(tagValue))
                {
                    node.Value.Add(tagValue);
                }
            }
            else
            {
                var newNode = new KeyValuePair<string, List<string>>(tagName, new List<string>());
                newNode.Value.Add(tagValue);
                this.tags.Add(newNode);
            }
        }

        public void AddTag(string tagName, List<string> tagValues)
        {
            var node = tags.Find((i) =>
            {
                if (i.Key.Equals(tagName))
                {
                    return true;
                }
                else
                {
                    return false;
                }

            });
            if (node.Key != null && node.Key.Equals(tagName))
            {
                foreach (var i in tagValues)
                {
                    if (!node.Value.Contains(i))
                    {
                        node.Value.Add(i);
                    }
                }
            }
            else
            {
                var newNode = new KeyValuePair<string, List<string>>(tagName, new List<string>());
                newNode.Value.AddRange(tagValues);
                this.tags.Add(newNode);
            }

        }

        public bool IsEmpty()
        {
            return tags.Count == 0;
        }

        public string ToJsonString()
        {
            if (tags.Count == 0)
            {
                return "{}";
            }
            else
            {
                StringBuilder sb = new StringBuilder();
                StringWriter sw = new StringWriter(sb);
                using (JsonWriter writer = new JsonTextWriter(sw))
                {
                    writer.Formatting = Formatting.Indented;

                    writer.WriteStartObject();
                    {
                        foreach (var tag in tags)
                        {
                            writer.WritePropertyName(tag.Key);
                            writer.WriteStartArray();
                            {
                                foreach (var value in tag.Value)
                                {
                                    writer.WriteValue(value);
                                }
                            }
                            writer.WriteEndArray();
                        }
                    }
                    writer.WriteEndObject();
                }
                return sb.ToString();
            }
        }


    }
}
