-- phpMyAdmin SQL Dump
-- version 4.6.5.2
-- https://www.phpmyadmin.net/
--
-- Host: localhost
-- Generation Time: Jan 16, 2017 at 08:36 AM
-- Server version: 10.1.20-MariaDB
-- PHP Version: 7.0.14

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `officenv`
--

-- --------------------------------------------------------

--
-- Table structure for table `data`
--

CREATE TABLE `data` (
  `recID` int(10) UNSIGNED NOT NULL,
  `devID` int(10) UNSIGNED NOT NULL DEFAULT '0',
  `date` text NOT NULL,
  `time` text NOT NULL,
  `attr1` float NOT NULL DEFAULT '0',
  `attr2` float NOT NULL DEFAULT '0',
  `attr3` float NOT NULL DEFAULT '0',
  `attr4` float NOT NULL DEFAULT '0',
  `attr5` float NOT NULL DEFAULT '0',
  `attr6` float NOT NULL DEFAULT '0',
  `attr7` float NOT NULL DEFAULT '0',
  `attr8` float NOT NULL DEFAULT '0',
  `attr9` float NOT NULL DEFAULT '0',
  `attr10` float NOT NULL DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `devices`
--

CREATE TABLE `devices` (
  `devID` int(10) UNSIGNED NOT NULL,
  `phyAddr` text,
  `type` smallint(6) NOT NULL DEFAULT '0',
  `status` smallint(6) NOT NULL DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Dumping data for table `devices`
--

INSERT INTO `devices` (`devID`, `phyAddr`, `type`, `status`) VALUES
(1, 'aaaaaaaa', 1, 1),
(2, 'bbbbbbbb', 1, 1),
(3, 'cccccccc', 1, 1),
(4, 'dddddddd', 1, 1);

--
-- Indexes for dumped tables
--

--
-- Indexes for table `data`
--
ALTER TABLE `data`
  ADD PRIMARY KEY (`recID`),
  ADD KEY `devID` (`devID`);

--
-- Indexes for table `devices`
--
ALTER TABLE `devices`
  ADD PRIMARY KEY (`devID`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `data`
--
ALTER TABLE `data`
  MODIFY `recID` int(10) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=72179;
--
-- AUTO_INCREMENT for table `devices`
--
ALTER TABLE `devices`
  MODIFY `devID` int(10) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=5;
--
-- Constraints for dumped tables
--

--
-- Constraints for table `data`
--
ALTER TABLE `data`
  ADD CONSTRAINT `data_ibfk_1` FOREIGN KEY (`devID`) REFERENCES `devices` (`devID`);

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
