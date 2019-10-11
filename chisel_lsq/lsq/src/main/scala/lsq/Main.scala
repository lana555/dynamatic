
package lsq

import chisel3._
import chisel3.util._
import scala.util.parsing.json._
import lsq.config.LsqConfigs

object Main extends App {
  //JSON parser configuration
  JSON.globalNumberParser = {
    in =>
      try in.toInt catch {
        case _: NumberFormatException => in.toDouble
      }
  }

  class CC[T] {
    def unapply(a: Any): Option[T] = Some(a.asInstanceOf[T])
  }

  object M extends CC[Map[String, Any]]

  object L extends CC[List[Any]]

  object S extends CC[String]

  object I extends CC[Int]

  object LL extends CC[List[List[Int]]]

  object LI extends CC[List[Int]]


  //parse command line arguments
  val usage =
    """
    Usage: java -jar [-Xmx7G]  lsq.jar [--target-dir  target_dir]  --spec-file  spec_file.json
  """
  if (args.length == 0) println(usage)
  val arglist = args.toList
  type OptionMap = Map[Symbol, String]

  def nextOption(map: OptionMap, list: List[String]): OptionMap = {
    list match {
      case Nil => map
      case "--spec-file" :: value :: tail =>
        nextOption(map ++ Map('specFile -> value), tail)
      case "--target-dir" :: value :: tail =>
        nextOption(map ++ Map('targetFolder -> value), tail)
      /*case string :: opt2 :: tail if isSwitch(opt2) =>
        nextOption(map ++ Map('infile -> string), list.tail)*/
      case string :: Nil => nextOption(map ++ Map('infile -> string), list.tail)
      case option :: _ => println("Unknown option " + option)
        map
    }
  }

  val options = nextOption(Map(), arglist)

  if (!options.contains('specFile)) {
    println("--spec-file argument is mandatory!")
    println(usage)
    System.exit(1)
  }

  //read the spec file
  val source = scala.io.Source.fromFile(options('specFile))
  val jsonString = try source.mkString finally source.close()

  val parsed = JSON.parseFull(jsonString)

  //parse specification
  for {
    Some(M(map)) <- List(JSON.parseFull(jsonString))
    L(tests) = map("specifications")
    M(test) <- tests
    S(name) = test("name")
    S(accessType) = test("accessType")
    S(speculation) = test("speculation")
    I(dataWidth) = test("dataWidth")
    I(addrWidth) = test("addrWidth")
    I(fifoDepth) = test("fifoDepth")
    I(numLoadPorts) = test("numLoadPorts")
    I(numStorePorts) = test("numStorePorts")
    I(numBBs) = test("numBBs")
    I(bufferDepth) = test("bufferDepth")
    LL(loadOffsets) = test("loadOffsets")
    LL(storeOffsets) = test("storeOffsets")
    LL(loadPorts) = test("loadPorts")
    LL(storePorts) = test("storePorts")
    LI(numLoads) = test("numLoads")
    LI(numStores) = test("numStores")
  } yield {
    val config = new LsqConfigs(
      dataWidth = dataWidth,
      addrWidth = addrWidth,
      fifoDepth = fifoDepth,
      numLoadPorts = numLoadPorts,
      numStorePorts = numStorePorts,
      numBBs = numBBs,
      loadOffsets = loadOffsets,
      storeOffsets = storeOffsets,
      loadPorts = loadPorts,
      storePorts = storePorts,
      numLoads = numLoads,
      numStores = numStores,
      accessType = accessType,
      bufferDepth = bufferDepth,
      name = name
    )
    var args: Array[String] = Array("--target-dir", options.getOrElse('targetFolder, name + "_" + fifoDepth))
    //generate LSQ
    if(accessType == "AXI") {
      chisel3.Driver.execute(args, () => new LSQAXI(config))
    }

    if(accessType == "BRAM"){
      chisel3.Driver.execute(args, () => new LSQBRAM(config))
    }
  }
}
