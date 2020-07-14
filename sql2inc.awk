# state=0 have not seen any statement beginning yet, may be preamble of
#         content file
# state=1 seen begin of statement with name
# state=2 after first statement file
# state=3 seen empty line

/^--.*/ {
	  if(state > 0) {
            if(match(content,".*;\\\\n\"$")) content = substr(content,1,length(content)-4) "\"";
            if(match(content,".*;\"$")) content = substr(content,1,length(content)-2) "\"";
	    if(insertnewline) printf("\n"); else insertnewline=1;
	    printf("const char* const %s%s%s[] = {\n%s,\n\n  NULL };\n",prefix,name,postfix,content);
	  }
	  name = $2;
	  state = 1;
	  gsub(/-/, "_", name);
	  map[name] = prefix name
	}
/^[ \t]*$/ {
	  state = 3;
	}
/^[^-]/ {
	  gsub(/\\/, "\\\\");
	  gsub(/"/, "\\\"");
	  if(state==1) {
	    content = "  \"" $0 "\\n\"";
	    state = 2;
	  } else if(state==2) {
	    content = content "\n  \"" $0 "\\n\"";
	  } else if(state==3) {
            # if ended with semicolon remove that
	    if(match(content,".*;\\\\n\"$")) content = substr(content,1,length(content)-4) "\"";
            if(match(content,".*;\"$")) content = substr(content,1,length(content)-2) "\"";
	    content = content ",\n\n  \"" $0 "\\n\"";
            state = 2;
	  }
	}
END     {
          if(state > 0) {
            if(match(content,".*;\\\\n\"$")) content = substr(content,1,length(content)-4) "\"";
            if(match(content,".*;\"$")) content = substr(content,1,length(content)-2) "\"";
	    if(insertnewline) printf("\n"); else insertnewline=1;
	    printf("const char* const %s%s%s[] = {\n%s,\n\n  NULL };\n",prefix,name,postfix,content);
	    if (queryarray) {
              printf("\nconst char* const* %s[] = {\n",queryarray);
              for (v in map) {
                printf("  %s%s,\n",map[v],postfix);
              }
              printf("  NULL \n};\n");
	      printf("\n");
	      i = 0;
	      for (v in map) {
	        printf("const char* const** %s = &%s[%d];\n",map[v],queryarray,i);
	        i += 1;
	      }
            }
	  }
        }
