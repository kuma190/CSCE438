import java.io.IOException;
import java.util.StringTokenizer;
import java.lang.String;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;

public class HourTweets {

  public static class TokenizerMapper
       extends Mapper<Object, Text, Text, IntWritable>{

    private final static IntWritable one = new IntWritable(1);
    private Text word = new Text();

    public void map(Object key, Text value, Context context
                    ) throws IOException, InterruptedException {
    StringTokenizer itr = new StringTokenizer(value.toString(),"\n");
      while (itr.hasMoreTokens()) {
        //get hour of day 
          String line = itr.nextToken();
          if (line.length() != 0 && line.charAt(0)=='T'){
             // System.out.println(line);
              StringTokenizer tk = new StringTokenizer(line);
              if (tk.hasMoreTokens()) tk.nextToken();
              if (tk.hasMoreTokens()) tk.nextToken();
              if (tk.hasMoreTokens()){
                  String timestamp = tk.nextToken();
                  String aKey = timestamp.substring(0,2);
                  word.set(aKey);
                  context.write(word, one);
              }
          }
        
      }

    }
  }

  public static class IntSumReducer
       extends Reducer<Text,IntWritable,Text,IntWritable> {
    private IntWritable result = new IntWritable();

    public void reduce(Text key, Iterable<IntWritable> values,
                       Context context
                       ) throws IOException, InterruptedException {
      int sum = 0;
      for (IntWritable val : values) {
        sum += val.get();
      }
      result.set(sum);
      context.write(key, result);
    }
  }

  public static void main(String[] args) throws Exception {
    Configuration conf = new Configuration();
    conf.set("textinputformat.record.delimiter","\n");
    Job job = Job.getInstance(conf, "word count");
    //conf.set("textinputformat.record.delimiter", "\n\n");
    job.setJarByClass(HourTweets.class);
    job.setMapperClass(TokenizerMapper.class);
    job.setCombinerClass(IntSumReducer.class);
    job.setReducerClass(IntSumReducer.class);
    job.setOutputKeyClass(Text.class);
    job.setOutputValueClass(IntWritable.class);
    FileInputFormat.addInputPath(job, new Path(args[0]));
    FileOutputFormat.setOutputPath(job, new Path(args[1]));
    System.exit(job.waitForCompletion(true) ? 0 : 1);
  }
}

