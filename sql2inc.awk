/^--.*/ { if(s>0) { if(f==0) f=1; else printf("\n"); }
          if(match(c,".*;\\\\n\"$")) c = substr(c,1,length(c)-4) "\"";
          if(match(c,".*;\"$")) c = substr(c,1,length(c)-2) "\"";
          if(s>0&&s<=3) {
            printf("const char* const %s = \n%s;\n",n,c);
          } else if(s>3) {
            printf("const char* const %s[] = [\n%s\n\n  NULL ];\n",n,c);
          }
          n=$2; s=1;
          m[n] = n
        }
/^ *$/  { if(s==2) s=3; next;
        }
/^[^-]/ {
          gsub("\\", "\\\\");
          gsub("\"", "\\\"");
          if(s==1) {
            c="  \"" $0 "\\n\"";
            s=2;
          } else if(s==2) {
            c=c "\n  \"" $0 "\\n\"";
          } else if(s==3) {
            if(match(c,".*;\\\\n\"$")) c = substr(c,1,length(c)-4) "\"";
            c=c ",\n\n  \"" $0 "\\n\"";
            m[n] = "*" m[n]; 
            s=4;
          } else if(s==4) {
            c=c "\n  \"" $0 "\"";
          }
        }
END     { if(s>0) { if(f==0) f=1; else printf("\n"); }
          if(match(c,".*;\\\\n\"$")) c = substr(c,1,length(c)-4) "\"";
          if(match(c,".*;\"$")) c = substr(c,1,length(c)-2) "\"";
          if(s>0&&s<=3) {
            printf("const char* const %s = \n%s;\n",n,c);
          } else if(s>3) {
            printf("const char* const %s[] = [\n%s\n\n  NULL ];\n",n,c);
          }
          if(a) {
            printf("\nconst char* %s[][2] = {\n",a);
            for (v in m) {
              printf("  { \"%s\", %s },\n",v,m[v]);
            }
              printf("  { NULL, NULL }\n};\n");
          }
        }
